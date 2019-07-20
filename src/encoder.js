
var LinearBufferEncoderCountType = Object.freeze({
        Uint8  : 0,
        Uint16 : 1,
        Uint32 : 2,
        Uint64 : 3,
        uint8  : 0,
        uint16 : 1,
        uint32 : 2,
        uint64 : 3
})

function LinearBufferEncoderCountTypeSize (countType)
{
        if (countType == LinearBufferEncoderCountType.Uint8 ||
            countType == LinearBufferEncoderCountType.uint8) {
               return 1;
        } else if (countType == LinearBufferEncoderCountType.Uint16 ||
                   countType == LinearBufferEncoderCountType.uint16) {
               return 2;
        } else if (countType == LinearBufferEncoderCountType.Uint32 ||
                   countType == LinearBufferEncoderCountType.uint32) {
               return 4;
        } else if (countType == LinearBufferEncoderCountType.Uint64 ||
                   countType == LinearBufferEncoderCountType.uint64) {
               return 8;
        } else {
               return 0;
        }
}

var LinearBufferEncoderOffsetType = Object.freeze({
        Uint8  : 0,
        Uint16 : 1,
        Uint32 : 2,
        Uint64 : 3,
        uint8  : 0,
        uint16 : 1,
        uint32 : 2,
        uint64 : 3
})

function LinearBufferEncoderOffsetTypeSize (offsetType)
{
        if (offsetType == LinearBufferEncoderOffsetType.Uint8 ||
            offsetType == LinearBufferEncoderOffsetType.uint8) {
               return 1;
        } else if (offsetType == LinearBufferEncoderOffsetType.Uint16 ||
                   offsetType == LinearBufferEncoderOffsetType.uint16) {
               return 2;
        } else if (offsetType == LinearBufferEncoderOffsetType.Uint32 ||
                   offsetType == LinearBufferEncoderOffsetType.uint32) {
               return 4;
        } else if (offsetType == LinearBufferEncoderOffsetType.Uint64 ||
                   offsetType == LinearBufferEncoderOffsetType.uint64) {
               return 8;
        } else {
               return 0;
        }
}

function LineaBuffersEncoderCreateOptions () {
        this.emitterFunction = null;
        this.emitterContext  = null;
}

function LineaBuffersEncoderResetOptions () {
        this.emitterFunction = null;
        this.emitterContext  = null;
}

var LinearBufferEncoderEntryType = Object.freeze({
        Unknown: 0,
        Table  : 1,
        Vector : 2
})

function LinearBuffersEncoder (createOptions = null) {
        this.__entries         = new Array();
        this.__emitterFunction = this.emitterFunction;
        this.__emitterContext  = null;
        this.__emitterOffset   = 0;
        this.__outputBuffer    = new Uint8Array(0);
        this.__outputLength    = 0;
        this.__outputSize      = 0;
        if (createOptions != null) {
                if (createOptions.emitterFunction != null) {
                        this.__emitterFunction = createOptions.emitterFunction;
                        this.__emitterContext  = createOptions.emitterContext;
                }
        }
}

LinearBuffersEncoder.prototype.emitterFunction = function (context, offset, buffer, length) {
        console.log("default emitterFunction offset: ", offset, ", buffer: ", buffer, ", length: ", length);
        if (length < 0) {
                this.__outputLength = offset + length;
                return;
        }
        if (this.__outputSize < offset + length) {
                var i;
                var tmp;
                var size;
                size = (((offset + length) + 4095) / 4096) * 4096;
                tmp = new Uint8Array(size);
                for (i = 0; i < this.__outputLength; i++) {
                        tmp[i] = this.__outputBuffer[i];
                }
                delete this.__outputBuffer;
                this.__outputBuffer = tmp;
                this.__outputSize   = size;
        }
        if (buffer == null) {
                this.__outputBuffer.fill(0, offset, length);
        } else {
                for (i = 0; i < length; i++) {
                        this.__outputBuffer[offset + i] = buffer[i];
                }
        }
        this.__outputLength = Math.max(this.__outputSize, offset + length);
        console.log("__outputBuffer: ", this.__outputBuffer);
        return 0;
}

LinearBuffersEncoder.prototype.reset = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.tableStart = function (countType, offsetType, elements, size) {
        var entry;
        entry                 = new Object();
        entry.__type          = LinearBufferEncoderEntryType.Table;
        entry.__countType     = countType;
        entry.__countSize     = LinearBufferEncoderCountTypeSize(countType);
        entry.__offsetType    = offsetType
        entry.__offsetSize    = LinearBufferEncoderOffsetTypeSize(offsetType);
        entry.__offset        = this.__emitterOffset;
        entry.__elements      = elements;
        entry.__size          = size;
        entry.__presentBytes  = Math.floor((elements + 7) / 8);
        entry.__presentBuffer = new Uint8Array(this.__presentBytes);
        entry.__presentBuffer.fill(0);
        this.__emitterFunction(this.__emitterContext, entry.__offset, null, entry.__countSize + entry.__presentBytes + size);
        this.__emitterOffset += entry.__countSize + entry.__presentBytes + size;
        this.__entries.push(entry);
}

LinearBuffersEncoder.prototype.tableEnd = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.tableCancel = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.tableSetInt8 = function (element, offset, value) {
        var rc;
        var parent;
        var valueArray;
        if (this.__entries.length <= 0) {
                throw("logic error: entries is empty");
        }
        parent = this.__entries[this.__entries.length - 1];
        if (parent == undefined) {
                throw("logic error: parent is invalid");
        }
        if (parent.__type != LinearBufferEncoderEntryType.Table) {
                throw("logic error: parent is invalid")
        }
        if (element >= parent.__elements) {
                throw("logic error: element is invalid")
        }
        valueArray = new Int8Array(1);
        valueArray[0] = value;
        rc = this.__emitterFunction(this.__emitterContext, parent.__offset + parent.__countSize + parent.__presentBytes + offset, new Uint8Array(valueArray.buffer), Int8Array.BYTES_PER_ELEMENT);
        if (rc != 0) {
                throw("can not emit table element")
        }
        parent.__presentBuffer[element / 8] |= 1 << (element % 8);
        return 0;
}

LinearBuffersEncoder.prototype.tableSetInt16 = function (element, offset, value) {
        var rc;
        var parent;
        var valueArray;
        if (this.__entries.length <= 0) {
                throw("logic error: entries is empty");
        }
        parent = this.__entries[this.__entries.length - 1];
        if (parent == undefined) {
                throw("logic error: parent is invalid");
        }
        if (parent.__type != LinearBufferEncoderEntryType.Table) {
                throw("logic error: parent is invalid")
        }
        if (element >= parent.__elements) {
                throw("logic error: element is invalid")
        }
        valueArray = new Int16Array(1);
        valueArray[0] = value;
        rc = this.__emitterFunction(this.__emitterContext, parent.__offset + parent.__countSize + parent.__presentBytes + offset, new Uint8Array(valueArray.buffer), Int16Array.BYTES_PER_ELEMENT);
        if (rc != 0) {
                throw("can not emit table element")
        }
        parent.__presentBuffer[element / 8] |= 1 << (element % 8);
        return 0;
}

LinearBuffersEncoder.prototype.tableSetInt32 = function (element, offset, value) {
        var rc;
        var parent;
        var valueArray;
        if (this.__entries.length <= 0) {
                throw("logic error: entries is empty");
        }
        parent = this.__entries[this.__entries.length - 1];
        if (parent == undefined) {
                throw("logic error: parent is invalid");
        }
        if (parent.__type != LinearBufferEncoderEntryType.Table) {
                throw("logic error: parent is invalid")
        }
        if (element >= parent.__elements) {
                throw("logic error: element is invalid")
        }
        valueArray = new Int32Array(1);
        valueArray[0] = value;
        rc = this.__emitterFunction(this.__emitterContext, parent.__offset + parent.__countSize + parent.__presentBytes + offset, new Uint8Array(valueArray.buffer), Int32Array.BYTES_PER_ELEMENT);
        if (rc != 0) {
                throw("can not emit table element")
        }
        parent.__presentBuffer[element / 8] |= 1 << (element % 8);
        return 0;
}

LinearBuffersEncoder.prototype.tableSetInt64 = function (element, offset, value) {
        var rc;
        var parent;
        var valueArray;
        if (this.__entries.length <= 0) {
                throw("logic error: entries is empty");
        }
        parent = this.__entries[this.__entries.length - 1];
        if (parent == undefined) {
                throw("logic error: parent is invalid");
        }
        if (parent.__type != LinearBufferEncoderEntryType.Table) {
                throw("logic error: parent is invalid")
        }
        if (element >= parent.__elements) {
                throw("logic error: element is invalid")
        }
        valueArray = new BigInt64Array(1);
        valueArray[0] = BigInt(value);
        rc = this.__emitterFunction(this.__emitterContext, parent.__offset + parent.__countSize + parent.__presentBytes + offset, new Uint8Array(valueArray.buffer), BigInt64Array.BYTES_PER_ELEMENT);
        if (rc != 0) {
                throw("can not emit table element")
        }
        parent.__presentBuffer[element / 8] |= 1 << (element % 8);
        return 0;
}

LinearBuffersEncoder.prototype.tableSetUint8 = function (element, offset, value) {
        var rc;
        var parent;
        var valueArray;
        if (this.__entries.length <= 0) {
                throw("logic error: entries is empty");
        }
        parent = this.__entries[this.__entries.length - 1];
        if (parent == undefined) {
                throw("logic error: parent is invalid");
        }
        if (parent.__type != LinearBufferEncoderEntryType.Table) {
                throw("logic error: parent is invalid")
        }
        if (element >= parent.__elements) {
                throw("logic error: element is invalid")
        }
        valueArray = new Uint8Array(1);
        valueArray[0] = value;
        rc = this.__emitterFunction(this.__emitterContext, parent.__offset + parent.__countSize + parent.__presentBytes + offset, new Uint8Array(valueArray.buffer), Uint8Array.BYTES_PER_ELEMENT);
        if (rc != 0) {
                throw("can not emit table element")
        }
        parent.__presentBuffer[element / 8] |= 1 << (element % 8);
        return 0;
}

LinearBuffersEncoder.prototype.tableSetUint16 = function (element, offset, value) {
        var rc;
        var parent;
        var valueArray;
        if (this.__entries.length <= 0) {
                throw("logic error: entries is empty");
        }
        parent = this.__entries[this.__entries.length - 1];
        if (parent == undefined) {
                throw("logic error: parent is invalid");
        }
        if (parent.__type != LinearBufferEncoderEntryType.Table) {
                throw("logic error: parent is invalid")
        }
        if (element >= parent.__elements) {
                throw("logic error: element is invalid")
        }
        valueArray = new Uint16Array(1);
        valueArray[0] = value;
        rc = this.__emitterFunction(this.__emitterContext, parent.__offset + parent.__countSize + parent.__presentBytes + offset, new Uint8Array(valueArray.buffer), Uint16Array.BYTES_PER_ELEMENT);
        if (rc != 0) {
                throw("can not emit table element")
        }
        parent.__presentBuffer[element / 8] |= 1 << (element % 8);
        return 0;
}

LinearBuffersEncoder.prototype.tableSetUint32 = function (element, offset, value) {
        var rc;
        var parent;
        var valueArray;
        if (this.__entries.length <= 0) {
                throw("logic error: entries is empty");
        }
        parent = this.__entries[this.__entries.length - 1];
        if (parent == undefined) {
                throw("logic error: parent is invalid");
        }
        if (parent.__type != LinearBufferEncoderEntryType.Table) {
                throw("logic error: parent is invalid")
        }
        if (element >= parent.__elements) {
                throw("logic error: element is invalid")
        }
        valueArray = new Uint32Array(1);
        valueArray[0] = value;
        rc = this.__emitterFunction(this.__emitterContext, parent.__offset + parent.__countSize + parent.__presentBytes + offset, new Uint8Array(valueArray.buffer), Uint32Array.BYTES_PER_ELEMENT);
        if (rc != 0) {
                throw("can not emit table element")
        }
        parent.__presentBuffer[element / 8] |= 1 << (element % 8);
        return 0;
}

LinearBuffersEncoder.prototype.tableSetUint64 = function (element, offset, value) {
        var rc;
        var parent;
        var valueArray;
        if (this.__entries.length <= 0) {
                throw("logic error: entries is empty");
        }
        parent = this.__entries[this.__entries.length - 1];
        if (parent == undefined) {
                throw("logic error: parent is invalid");
        }
        if (parent.__type != LinearBufferEncoderEntryType.Table) {
                throw("logic error: parent is invalid")
        }
        if (element >= parent.__elements) {
                throw("logic error: element is invalid")
        }
        valueArray = new BigUint64Array(1);
        valueArray[0] = BigInt(value);
        rc = this.__emitterFunction(this.__emitterContext, parent.__offset + parent.__countSize + parent.__presentBytes + offset, new Uint8Array(valueArray.buffer), BigUint64Array.BYTES_PER_ELEMENT);
        if (rc != 0) {
                throw("can not emit table element")
        }
        parent.__presentBuffer[element / 8] |= 1 << (element % 8);
        return 0;
}

LinearBuffersEncoder.prototype.tableSetFloat = function (element, offset, value) {
        var rc;
        var parent;
        var valueArray;
        if (this.__entries.length <= 0) {
                throw("logic error: entries is empty");
        }
        parent = this.__entries[this.__entries.length - 1];
        if (parent == undefined) {
                throw("logic error: parent is invalid");
        }
        if (parent.__type != LinearBufferEncoderEntryType.Table) {
                throw("logic error: parent is invalid")
        }
        if (element >= parent.__elements) {
                throw("logic error: element is invalid")
        }
        valueArray = new Float32Array(1);
        valueArray[0] = value;
        rc = this.__emitterFunction(this.__emitterContext, parent.__offset + parent.__countSize + parent.__presentBytes + offset, new Uint8Array(valueArray.buffer), Float32Array.BYTES_PER_ELEMENT);
        if (rc != 0) {
                throw("can not emit table element")
        }
        parent.__presentBuffer[element / 8] |= 1 << (element % 8);
        return 0;
}

LinearBuffersEncoder.prototype.tableSetDouble = function (element, offset, value) {
        var rc;
        var parent;
        var valueArray;
        if (this.__entries.length <= 0) {
                throw("logic error: entries is empty");
        }
        parent = this.__entries[this.__entries.length - 1];
        if (parent == undefined) {
                throw("logic error: parent is invalid");
        }
        if (parent.__type != LinearBufferEncoderEntryType.Table) {
                throw("logic error: parent is invalid")
        }
        if (element >= parent.__elements) {
                throw("logic error: element is invalid")
        }
        valueArray = new Float64Array(1);
        valueArray[0] = value;
        rc = this.__emitterFunction(this.__emitterContext, parent.__offset + parent.__countSize + parent.__presentBytes + offset, new Uint8Array(valueArray.buffer), Float64Array.BYTES_PER_ELEMENT);
        if (rc != 0) {
                throw("can not emit table element")
        }
        parent.__presentBuffer[element / 8] |= 1 << (element % 8);
        return 0;
}

LinearBuffersEncoder.prototype.tableSetString = function (element, offset, value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.tableSetTable = function (element, offset, value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.tableSetVector = function (element, offset, value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.stringCreate = function (element, value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorStartInt8 = function (countType, offsetType) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorEndInt8 = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCancelInt8 = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorPushInt8 = function (value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCreateInt8 = function (countType, offsetType, value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorStartInt16 = function (countType, offsetType) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorEndInt16 = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCancelInt16 = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorPushInt16 = function (value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCreateInt16 = function (countType, offsetType, value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorStartInt32 = function (countType, offsetType) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorEndInt32 = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCancelInt32 = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorPushInt32 = function (value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCreateInt32 = function (countType, offsetType, value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorStartInt64 = function (countType, offsetType) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorEndInt64 = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorStartUint8 = function (countType, offsetType) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorEndUint8 = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCancelUint8 = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorPushUint8 = function (value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCreateUint8 = function (countType, offsetType, value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorStartUint16 = function (countType, offsetType) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorEndUint16 = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCancelUint16 = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorPushUint16 = function (value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCreateUint16 = function (countType, offsetType, value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorStartUint32 = function (countType, offsetType) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorEndUint32 = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCancelUint32 = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorPushUint32 = function (value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCreateUint32 = function (countType, offsetType, value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorStartUint64 = function (countType, offsetType) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorEndUint64 = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCancelUint64 = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorPushUint64 = function (value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCreateUint64 = function (countType, offsetType, value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorStartFloat = function (countType, offsetType) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorEndFloat = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCancelFloat = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorPushFloat = function (value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCreateFloat = function (countType, offsetType, value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorStartDouble = function (countType, offsetType) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorEndDouble = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCancelDouble = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorPushDouble = function (value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCreateDouble = function (countType, offsetType, value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorStartString = function (countType, offsetType) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorEndString = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCancelString = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorPushString = function (value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCreateString = function (countType, offsetType, value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorStartTable = function (countType, offsetType) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorEndTable = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCancelTable = function () {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorPushTable = function (value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.vectorCreateTable = function (countType, offsetType, value) {
        throw("not implemented yet");
}

LinearBuffersEncoder.prototype.linearized = function () {
        return this.__outputBuffer;
}

LinearBuffersEncoder.prototype.LinearBufferEncoderCountType = LinearBufferEncoderCountType;

LinearBuffersEncoder.prototype.LinearBufferEncoderOffsetType = LinearBufferEncoderOffsetType;

module.exports = {
        LinearBufferEncoderCountType : LinearBufferEncoderCountType,
        LinearBufferEncoderOffsetType: LinearBufferEncoderOffsetType,
        LinearBuffersEncoder         : LinearBuffersEncoder
}
