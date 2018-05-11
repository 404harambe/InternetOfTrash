const express = require('express');
const mongoose = require('mongoose');
const restify = require('express-restify-mongoose');
const { Bin, Measurement } = require('./models');

const router = express.Router();

// Use express-restify-mongoose to expose a full REST interface for bins
restify.serve(router, Bin, {
    prefix: '',
    version: '',
    private: [ '__v' ],
    postRead: (req, res, next) => {
        const result = req.erm.result;
        const statusCode = req.erm.statusCode;
      
        // Add to each bin the most recent measurement
        Promise.all(
            result.map(bin =>
                Measurement.findOne({ binId: bin._id }).sort({ timestamp: 'desc' }).exec()
                    .then(m => ({ ...bin, lastMeasurement: m }))
            )
        )
        .then(
            res => {
                req.erm.result = res;
                next();
            },
            err => next(err)
        );
    },
    outputFn: (req, res) => {
        const { statusCode, result } = req.erm;
        if (!statusCode || (statusCode >= 200 && statusCode < 300)) {
            res.sendSuccess(result);
        } else {
            res.sendError();
        }
    },
    onError: (err, req, res, next) => {
        const { statusCode } = req.erm;
        if (statusCode == 400) {
            res.sendError('Invalid request.');
        } else if (statusCode == 404) {
            res.sendError('Not found.');
        } else {
            console.error(err);
            res.sendError();
        }
    }
});

// Route to force an update
router.post('/bin/:id/update', async (req, res, next) => {
    try {
        const bin = await Bin.findById(req.params.id).exec();
        if (bin) {
            
            // Send a message to the arduino controlling this bin
            if (bin.arduinoId > 0) {
                await req.mqttclient.publish('arduino/registered/' + bin.arduinoId, 'update');
            }

            res.sendSuccess(bin);

        } else {
            res.sendError('Not found.');
        }
    } catch (e) {
        next(e);
    }
});

// Route to allow bulk uploading of measurements.
// This endpoint allows to upload multiple measurements for different bins in a single request.
router.post('/bulkmeasurements', async (req, res, next) => {
    try {
        await Measurement.insertMany(req.body);
        res.sendSuccess();
    } catch (e) {
        if (e.name === 'ValidationError') {
            res.sendError('Invalid request.');
        } else {
            next(e);
        }
    }
});

module.exports = router;