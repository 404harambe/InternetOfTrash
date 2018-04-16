const mongoose = require('mongoose');
const Schema = mongoose.Schema;

const BinSchema = new Schema({
    _id: Schema.Types.ObjectId,
    type: {
        type: String,
        enum: [ 'plastic', 'paper', 'organic', 'glass', 'generic' ],
        default: 'generic'
    },
    address: String
});

module.exports = {
    Bin: mongoose.model('Bin', BinSchema)
};