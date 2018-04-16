const express = require('express');
const restify = require('express-restify-mongoose');
const { Bin } = require('./models');

const router = express.Router();

// Use express-restify-mongoose to expose a full REST interface for bins
restify.serve(router, Bin, {
    prefix: '',
    onError: (err, req, res, next) => {
        const statusCode = req.erm.statusCode // 400 or 404
        res.status(statusCode).json({
            error: statusCode == 400 ? 'Invalid request.' :
                   statusCode == 404 ? 'Not found.' :
                                       'Server error.'
            
        })
    }
})

module.exports = router;