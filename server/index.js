const express = require('express');
const bodyParser = require('body-parser');
const mongoose = require('mongoose');
const api = require('./api');
const config = require('./config');
const { Bin } = require('./models');
const mqtt = require('./mqtt');

const app = express();
const server = require('http').Server(app);
const io = require('socket.io')(server);

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
mqtt.mqttclient.on('connect', () => {

    console.log('MQTT broker connected.');
    
    // Connect to MongoDB
    mongoose.Promise = Promise;
    mongoose.connect(`mongodb://${config.mongo.host}:${config.mongo.port}/${config.mongo.db}`)
        .then(() => {
            console.log('MongoDB connected.');
            
            // Starts the HTTP server
            server.listen(config.server.listen_port, () => console.log(`InternetOfTrash server started on port ${config.server.listen_port}.`));
            
        })
        .catch(console.error.bind(console, 'MongoDB connection error:'));

});
mqtt.mqttclient.on('error', err => {
    console.error('MQTT broker connection error:', err);
});

// Redirect all new measurements as broadcast socket.io messages
mqtt.on('measurement', m => {
    io.emit('measurement', m.toJSON());
});