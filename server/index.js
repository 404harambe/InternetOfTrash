const express = require('express');
const bodyParser = require('body-parser');
const mongoose = require('mongoose');
const api = require('./api');

const MONGODB_CONNECTION_URI = 'mongodb://localhost:27017/internetoftrash';
const LISTEN_PORT = 8010;

// Setup express
const app = express();
app.use(bodyParser.json());
app.use('/api', api);
app.use((err, req, res, next) => {
    console.error(err);
    res.status(500).json({ error: 'An internal error occurred.' })
});

// Connect to the local MongoDB
mongoose.Promise = Promise;
mongoose.connect(MONGODB_CONNECTION_URI).then(
    () => app.listen(LISTEN_PORT, () => console.log(`InternetOfTrash server started on port ${LISTEN_PORT}.`)),
    console.error.bind(console, 'MongoDB connection error:')
);