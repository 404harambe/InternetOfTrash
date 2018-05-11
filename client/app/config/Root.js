import React from 'react'; // eslint-disable-line no-unused-vars
import { BrowserRouter as Router, Route, Switch } from 'react-router-dom';
import App from '../components/App';

export default () => (
    <Router>
        <Switch>
            <Route path="/" component={App} exact />
        </Switch>
    </Router>
);