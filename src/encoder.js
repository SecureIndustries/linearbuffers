
var LinearBufferEncoderCountType = Object.freeze({
        Uint8 : 0,
        Uint16: 1,
        Uint32: 2,
        Uint64: 3
})

var LinearBufferEncoderOffsetType = Object.freeze({
        Uint8 : 0,
        Uint16: 1,
        Uint32: 2,
        Uint64: 3  
})

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

LinearBuffersEncoder.prototype.emitterFunction = function (context, offset, buffer, length) {
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

module.exports = {
        LinearBufferEncoderCountType : LinearBufferEncoderCountType,
        LinearBufferEncoderOffsetType: LinearBufferEncoderOffsetType,
        LinearBuffersEncoder         : LinearBuffersEncoder
}