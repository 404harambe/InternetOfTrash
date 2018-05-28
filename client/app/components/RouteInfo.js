import React from 'react'; // eslint-disable-line
import BinStats from './BinStats';

function displaySeconds(d) {
    d = Number(d);
    const h = Math.floor(d / 3600);
    const m = Math.floor(d % 3600 / 60);
    const s = Math.floor(d % 3600 % 60);

    const hDisplay = h > 0 ? h + (h == 1 ? " hour, " : " hours, ") : "";
    const mDisplay = m > 0 ? m + (m == 1 ? " minute, " : " minutes, ") : "";
    const sDisplay = s > 0 ? s + (s == 1 ? " second" : " seconds") : "";
    return hDisplay + mDisplay + sDisplay; 
}

export default props => {

    const bins = props.bins;
    const routes = props.route;
    const route = routes.routes[0];

    // Compute some stats on the route
    const distance = route.legs.reduce((a, b) => a + b.distance.value, 0);
    const time = route.legs.reduce((a, b) => a + b.duration.value, 0);

    return (
        <div className="route-info">
            <h1>Route info</h1>
            <p>Collection center address: <strong>{routes.request.origin.query}</strong></p>
            <p>Total distance: <strong>{(distance / 1000).toFixed(2)}</strong> Km</p>
            <p>Estimated travel time: <strong>{displaySeconds(time)}</strong></p>
            
            <h1>{bins.length} {bins.length === 1 ? 'bin' : 'bins'} to collect</h1>
            <ol>
                {route.waypoint_order.map(i => {
                    const b = bins[i];
                    return (
                        <li key={i}>
                            <strong>{b.address}</strong>
                            <BinStats bin={b} />
                        </li>
                    );
                })}
            </ol>
        </div>
    );
};