const express = require('express');
const bodyParser = require('body-parser');
const mongoose = require('mongoose');
const api = require('./api');

const MONGODB_CONNECTION_URI = 'mongodb://localhost:27017/internetoftrash';
const LISTEN_PORT = 8010;

const app = express();

// Attaches two helper functions to send responses.
// They are needed to send responses (both 200s and errors)
// under the same JSON interface.
app.use((req, res, next) => {
    res.sendSuccess = function (obj) {
        res.status(200).send({
            status: 'ok',
            contents: obj
        });
    };
    res.sendError = function (error) {
        error = error || 'An internal error has occurred.';
        res.status(200).send({
            status: 'error',
            error: error
        });
    };
    next();
});

// Setup express
app.use(bodyParser.json());

// Mount all the routes
app.use('/api', api);
app.use((err, req, res, next) => {
    console.error(err);
    res.sendError();
});

// Connect to the local MongoDB
mongoose.Promise = Promise;
mongoose.connect(MONGODB_CONNECTION_URI).then(
    () => app.listen(LISTEN_PORT, () => console.log(`InternetOfTrash server started on port ${LISTEN_PORT}.`)),
    console.error.bind(console, 'MongoDB connection error:')
);