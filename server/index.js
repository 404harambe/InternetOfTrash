const express = require('express');
const bodyParser = require('body-parser');
const mongoose = require('mongoose');
const api = require('./api');
const config = require('./config');
const { Bin } = require('./models');
const { mqttclient } = require('./mqtt');

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

// Connect to the MQTT broker
mqttclient.on('connect', () => {

    console.log('MQTT broker connected.');
    
    // Connect to MongoDB
    mongoose.Promise = Promise;
    mongoose.connect(`mongodb://${config.mongo.host}:${config.mongo.port}/${config.mongo.db}`)
        .then(() => {
            console.log('MongoDB connected.');
            
            // Starts the HTTP server
            app.listen(config.server.listen_port, () => console.log(`InternetOfTrash server started on port ${config.server.listen_port}.`));
            
        })
        .catch(console.error.bind(console, 'MongoDB connection error:'));

});
mqttclient.on('error', err => {
    console.error('MQTT broker connection error:', err);
});