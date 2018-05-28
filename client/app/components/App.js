import React from 'react';
import FAIcon from '@fortawesome/react-fontawesome';
import { ToastContainer, toast } from 'react-toastify';
import Map from './Map';
import NavBar from './NavBar';
import RouteInfo from './RouteInfo';
import config from 'config';

export default class App extends React.Component {
    
    constructor(props) {
        super(props);
        this.io = io();
        this.computeRouteForBins = this.computeRouteForBins.bind(this);
        this.onMeasurementUpdate = this.onMeasurementUpdate.bind(this);
        this.state = {
            loading: true,
            bins: [],
            route: null,
            routeBins: null
        };
    }

    componentDidMount() {
        fetch('/api/bin')
            .then(res => res.json())
            .then(data => {
                if (data.status === 'ok') {
                    this.setState({ loading: false, bins: data.contents });
                    this.io.on('measurement', this.onMeasurementUpdate);
                }
            });
    }

    componentWillUnmount() {
        this.io.removeListener('measurement', this.onMeasurementUpdate);
    }

    onMeasurementUpdate(measurement) {

        // Update the last measurement of the corresponding bin
        let didUpdate = null;
        const newBins = this.state.bins.map(b => {
            if (b._id !== measurement.binId) {
                return b;
            }
            
            // eslint-disable console
            console.group('Got new update!');
            console.log('Measurement', measurement);
            console.log('Bin', b);
            console.groupEnd();
            // eslint-enable console

            //if (new Date(b.lastMeasurement.timestamp) < new Date(measurement.timestamp)) {
                b.lastMeasurement = measurement;
                didUpdate = b.address;
            //}
            return b;
        });

        if (didUpdate !== null) {
            toast.info("New live data for " + didUpdate, {
                position: toast.POSITION.BOTTOM_LEFT
            });
            this.setState({ bins: newBins });
        }
    }

    computeRouteForBins(type) {
        let { bins } = this.state;

        // Just remove the route if we've been requested to do so
        if (type === null) {
            this.setState({ route: null });
            return;
        }

        // Filter the bins
        if (type === 'full') { 
            bins = bins.filter(bin => {
                const level = (bin.height - (bin.lastMeasurement ? bin.lastMeasurement.value : bin.height)) / bin.height;
                return level >= 0.8;
            });
        } else if (type !== 'all') {
            bins = bins.filter(b => b.type === type);
        }

        // Map the bin locations to waypoints for the gmaps api
        const waypoints = bins.map(b => ({
            location: { lat: b.lat, lng: b.long },
            stopover: true
        }));

        // Request a route
        this.setState({ loading: true });
        const service = new google.maps.DirectionsService();
        service.route({
            origin: config.collection_point,
            destination: config.collection_point,
            travelMode: 'DRIVING',
            optimizeWaypoints: true,
            waypoints
        }, (res, status) => {
            if (status === 'OK') {
                this.setState({ loading: false, route: res, routeBins: bins });
            } else {
                this.setState({ loading: false, route: null, routeBins: bins });
            }
        });

    }

    render() {
        const { loading, bins, route, routeBins } = this.state;

        const contents = (
            <div style={{ width: '100%', height: '100%' }}>
                <div style={{ width: route ? '80%' : '100%', height: '100%', paddingTop: '56px', float: 'left' }}>

                    {/* Main map */}
                    <Map bins={bins} route={route} />

                </div>
                <div style={{ width: '20%', height: '100%', paddingTop: '56px', float: 'right', display: route ? 'block': 'none' }}>

                    {/* Side panel for route info */}
                    {route && (
                        <div style={{ width: '100%', height: '100%', overflow: 'auto', padding: '5px 15px' }}>
                            <RouteInfo route={route} bins={routeBins} />
                        </div>
                    )}

                </div>
                <div style={{ position: 'absolute', top: 0, left: 0, width: '100%' }}>

                    {/* Nav bar */}
                    <NavBar computeRouteForBins={this.computeRouteForBins} />

                </div>
            </div>
        );

        return (
            <div style={{ width: '100%', height: '100%' }}>
                {contents}

                {/* A huge spinner that blocks the page */}
                {loading && (
                    <div className="fadeIn" style={{ position: 'absolute', width: '100%', height: '100vh', top: 0, left: 0 }}>
                        <div style={{ position: 'absolute', top: 0, left: 0, width: '100%', height: '100%', background: 'white', opacity: 0.5 }} />
                        <div style={{ position: 'absolute', top: '50%', left: '50%', transform: 'translate(-50%, -50%)' }}>
                            <FAIcon icon="circle-notch" spin size="10x" color="#333" />
                        </div>
                    </div>
                )}

                <ToastContainer />
            </div>
        );
        
    }

}
