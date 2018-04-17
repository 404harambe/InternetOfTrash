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
})

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