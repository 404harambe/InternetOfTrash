import React from 'react'; // eslint-disable-line no-unused-vars
import ReactDOM from 'react-dom';
import { AppContainer } from 'react-hot-loader';

import Root from './config/Root';

import 'bootstrap/dist/css/bootstrap.min.css';
import '@fortawesome/fontawesome-free-solid';
import '@fortawesome/fontawesome-free-regular';
import 'react-toastify/dist/ReactToastify.min.css';

const render = (Component) => {
    ReactDOM.render(
        <AppContainer>
            <Component />
        </AppContainer>,
        document.getElementById('root'),
    );
};

render(Root);

if (module.hot) {
    module.hot.accept('./config/Root', () => {
        const newApp = require('./config/Root').default;
        render(newApp);
    });
}
