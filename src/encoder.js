
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

function LinearBufferEncoderCountOffsetSize (offsetType)
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

function LinearBuffersEncoderEntryTable () {
        this.__elements = 0;
        this.__size     = 0;
}

function LinearBuffersEncoderEntry () {
        this.__type       = LinearBufferEncoderEntryType.Unknown;
        this.__countSize  = 0;
        this.__offsetSize = 0;
        this.__offset     = 0;
        this.__table      = null;
        this.__vector     = null;
}

function LinearBuffersEncoder (createOptions = null) {
        this.__entries         = new Array();
        this.__emitterFunction = this.emitterFunction;
        this.__emitterContext  = null;
        this.__emitterOffset   = 0;
        this.__outputBuffer    = new Uint8Array(0);
        this.__outputLength    = 0;
        this.__outputSuze      = 0;
        if (createOptions != null) {
                if (createOptions.emitterFunction != null) {
                        this.__emitterFunction = createOptions.emitterFunction;
                        this.__emitterContext  = createOptions.emitterContext;
                }
        }
}

LinearBuffersEncoder.prototype.emitterFunction = function (context, offset, buffer, length)
{
        console.log("default emitterFunction");
}

LinearBuffersEncoder.prototype.reset = function () {
}

LinearBuffersEncoder.prototype.tableStart = function (countType, offsetType, elements, size) {
        var entry;
        entry                  = new LinearBuffersEncoderEntry();
        entry.__type           = LinearBufferEncoderEntryType.Table;
        entry.__countSize      = countType;
        entry.__offsetSize     = offsetType;
        entry.__offset         = this.__emitterOffset;
        entry.__table          = new LinearBuffersEncoderEntryTable();
        entry.__table.elements = elements;
        entry.__table.size     = size;
        this.__emitterFunction(this.__emitterContext, entry.__offset, NULL, entry.__countSize + 0 + size);
        this.__entries.push(entry);
}

LinearBuffersEncoder.prototype.tableEnd = function () {
}

LinearBuffersEncoder.prototype.tableCancel = function () {
}

LinearBuffersEncoder.prototype.tableSetInt8 = function (element, offset, value) {
}

LinearBuffersEncoder.prototype.tableSetInt16 = function (element, offset, value) {
}

LinearBuffersEncoder.prototype.tableSetInt32 = function (element, offset, value) {
}

LinearBuffersEncoder.prototype.tableSetInt64 = function (element, offset, value) {
}

LinearBuffersEncoder.prototype.tableSetUint8 = function (element, offset, value) {
}

LinearBuffersEncoder.prototype.tableSSetUin16 = function (element, offset, value) {
}

LinearBuffersEncoder.prototype.tableSSetUin32 = function (element, offset, value) {
}

LinearBuffersEncoder.prototype.tableSSetUin64 = function (element, offset, value) {
}

LinearBuffersEncoder.prototype.tableSSetFloat = function (element, offset, value) {
}

LinearBuffersEncoder.prototype.tableSSetDouble = function (element, offset, value) {
}

LinearBuffersEncoder.prototype.tableSSetString = function (element, offset, value) {
}

LinearBuffersEncoder.prototype.tableSSetTable = function (element, offset, value) {
}

LinearBuffersEncoder.prototype.tableSSetVector = function (element, offset, value) {
}

LinearBuffersEncoder.prototype.tableStringCreate = function (element, offset, value) {
}

LinearBuffersEncoder.prototype.vectorStartInt8 = function (countType, offsetType) {
}

LinearBuffersEncoder.prototype.vectorEndInt8 = function () {
}

LinearBuffersEncoder.prototype.vectorCancelInt8 = function () {
}

LinearBuffersEncoder.prototype.vectorPushInt8 = function (value) {
}

LinearBuffersEncoder.prototype.vectorCreateInt8 = function (countType, offsetType, value) {
}

LinearBuffersEncoder.prototype.vectorStartInt16 = function (countType, offsetType) {
}

LinearBuffersEncoder.prototype.vectorEndInt16 = function () {
}

LinearBuffersEncoder.prototype.vectorCancelInt16 = function () {
}

LinearBuffersEncoder.prototype.vectorPushInt16 = function (value) {
}

LinearBuffersEncoder.prototype.vectorCreateInt16 = function (countType, offsetType, value) {
}

LinearBuffersEncoder.prototype.vectorStartInt32 = function (countType, offsetType) {
}

LinearBuffersEncoder.prototype.vectorEndInt32 = function () {
}

LinearBuffersEncoder.prototype.vectorCancelInt32 = function () {
}

LinearBuffersEncoder.prototype.vectorPushInt32 = function (value) {
}

LinearBuffersEncoder.prototype.vectorCreateInt32 = function (countType, offsetType, value) {
}

LinearBuffersEncoder.prototype.vectorStartInt64 = function (countType, offsetType) {
}

LinearBuffersEncoder.prototype.vectorEndInt64 = function () {
}

LinearBuffersEncoder.prototype.vectorStartUint8 = function (countType, offsetType) {
}

LinearBuffersEncoder.prototype.vectorEndUint8 = function () {
}

LinearBuffersEncoder.prototype.vectorCancelUint8 = function () {
}

LinearBuffersEncoder.prototype.vectorPushUint8 = function (value) {
}

LinearBuffersEncoder.prototype.vectorCreateUint8 = function (countType, offsetType, value) {
}

LinearBuffersEncoder.prototype.vectorStartUint16 = function (countType, offsetType) {
}

LinearBuffersEncoder.prototype.vectorEndUint16 = function () {
}

LinearBuffersEncoder.prototype.vectorCancelUint16 = function () {
}

LinearBuffersEncoder.prototype.vectorPushUint16 = function (value) {
}

LinearBuffersEncoder.prototype.vectorCreateUint16 = function (countType, offsetType, value) {
}

LinearBuffersEncoder.prototype.vectorStartUint32 = function (countType, offsetType) {
}

LinearBuffersEncoder.prototype.vectorEndUint32 = function () {
}

LinearBuffersEncoder.prototype.vectorCancelUint32 = function () {
}

LinearBuffersEncoder.prototype.vectorPushUint32 = function (value) {
}

LinearBuffersEncoder.prototype.vectorCreateUint32 = function (countType, offsetType, value) {
}

LinearBuffersEncoder.prototype.vectorStartUint64 = function (countType, offsetType) {
}

LinearBuffersEncoder.prototype.vectorEndUint64 = function () {
}

LinearBuffersEncoder.prototype.vectorCancelUint64 = function () {
}

LinearBuffersEncoder.prototype.vectorPushUint64 = function (value) {
}

LinearBuffersEncoder.prototype.vectorCreateUint64 = function (countType, offsetType, value) {
}

LinearBuffersEncoder.prototype.vectorStartFloat = function (countType, offsetType) {
}

LinearBuffersEncoder.prototype.vectorEndFloat = function () {
}

LinearBuffersEncoder.prototype.vectorCancelFloat = function () {
}

LinearBuffersEncoder.prototype.vectorPushFloat = function (value) {
}

LinearBuffersEncoder.prototype.vectorCreateFloat = function (countType, offsetType, value) {
}

LinearBuffersEncoder.prototype.vectorStartDouble = function (countType, offsetType) {
}

LinearBuffersEncoder.prototype.vectorEndDouble = function () {
}

LinearBuffersEncoder.prototype.vectorCancelDouble = function () {
}

LinearBuffersEncoder.prototype.vectorPushDouble = function (value) {
}

LinearBuffersEncoder.prototype.vectorCreateDouble = function (countType, offsetType, value) {
}

LinearBuffersEncoder.prototype.vectorStartString = function (countType, offsetType) {
}

LinearBuffersEncoder.prototype.vectorEndString = function () {
}

LinearBuffersEncoder.prototype.vectorCancelString = function () {
}

LinearBuffersEncoder.prototype.vectorPushString = function (value) {
}

LinearBuffersEncoder.prototype.vectorCreateString = function (countType, offsetType, value) {
}

LinearBuffersEncoder.prototype.vectorStartTable = function (countType, offsetType) {
}

LinearBuffersEncoder.prototype.vectorEndTable = function () {
}

LinearBuffersEncoder.prototype.vectorCancelTable = function () {
}

LinearBuffersEncoder.prototype.vectorPushTable = function (value) {
}

LinearBuffersEncoder.prototype.vectorCreateTable = function (countType, offsetType, value) {
}

module.exports = {
        LinearBufferEncoderCountType : LinearBufferEncoderCountType,
        LinearBufferEncoderOffsetType: LinearBufferEncoderOffsetType,
        LinearBuffersEncoder         : LinearBuffersEncoder
}
