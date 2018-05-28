import React from 'react'; // eslint-disable-line no-unused-vars
import GoogleMapReact from 'google-map-react';
import equal from 'deep-equal';
import BinMarker from './BinMarker';
import config from 'config';

const pisa = { lat: 43.719799, lng: 10.405584 };

export default class Map extends React.Component {
    static defaultProps = {
        center: pisa,
        zoom: 16,
        bins: [],
        route: null
    };

    constructor(props) {
        super(props);
        this.onGoogleApiLoaded = this.onGoogleApiLoaded.bind(this);
    }

    onGoogleApiLoaded({ map }) {
        this._directionsRenderer = new google.maps.DirectionsRenderer();
        this._directionsRenderer.setMap(map);
        this._directionsRenderer.setOptions({
            suppressMarkers: true
        });
        if (this.props.route) {
            this._directionsRenderer.setDirections(this.props.route);
        }
    }

    componentWillUpdate(nextProps) {
        if (this._directionsRenderer && this.props.route != nextProps.route) {
            this._directionsRenderer.setDirections(nextProps.route || { routes: [] });
        }
    }

    render() {
        const { center, zoom, bins } = this.props;
        return (
            <GoogleMapReact
                bootstrapURLKeys={{ key: config.gmaps_key }}
                defaultCenter={center}
                defaultZoom={zoom}
                onGoogleApiLoaded={this.onGoogleApiLoaded}
                yesIWantToUseGoogleMapApiInternals>

                {bins.map((bin, i) => (
                    <BinMarker key={i} bin={bin} lat={bin.lat} lng={bin.long} />
                ))}

            </GoogleMapReact>
        );
    }
}