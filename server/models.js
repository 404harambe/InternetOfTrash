const mongoose = require('mongoose');
const Schema = mongoose.Schema;

const BinSchema = new Schema({
    type: {
        type: String,
        enum: [ 'plastic', 'paper', 'organic', 'glass', 'generic' ],
        default: 'generic',
        required: true
    },
    address: { type: String, required: true },
    lat: { type: Number, required: true },
    long: { type: Number, required: true },
    height: { type: Number, required: true }
});

const MeasurementSchema = new Schema({
    timestamp: { type: Date, required: true },
    binId: { type: Schema.Types.ObjectId, ref: 'Bin' },
    value: { type: Number, required: true }
});

module.exports = {
    Bin: mongoose.model('Bin', BinSchema),
    Measurement: mongoose.model('Measurement', MeasurementSchema)
};