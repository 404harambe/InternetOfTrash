import React from 'react';
import { Button, Popover, PopoverHeader, PopoverBody } from 'reactstrap';
import FAIcon from '@fortawesome/react-fontawesome';
import extend from 'extend';
import BinStats from './BinStats';

const binColors = {
    'plastic': '#0053ad',
    'organic': '#2d0707',
    'generic': '#333333',
    'paper': '#676508'
};

const backgroundColors = {
    'plastic': '#9cc7ff',
    'organic': '#633f3f',
    'generic': '#808080',
    'paper': '#ffe476'
};

export default class BinMarker extends React.Component {
    constructor(props) {
        super(props);

        this.toggle = this.toggle.bind(this);
        this.onUpdateClick = this.onUpdateClick.bind(this);

        this.state = {
            popoverOpen: false,
            updating: false,
            error: null
        };
    }

    toggle() {
        this.setState({
            popoverOpen: !this.state.popoverOpen
        });
    }

    showError(error) {
        this.setState({ error });
    }

    onUpdateClick() {
        this.setState({
            updating: true,
            error: null
        });

        const { bin } = this.props;
        fetch(`/api/bin/${bin._id}/update`, { method: 'POST' })
            .then(res => res.json())
            .then(
                data => {
                    if (data.status === 'ok') {
                        const newbin = data.contents;
                        extend(bin, newbin);
                        this.showError(null);
                    } else {
                        this.showError(data.error);
                    }
                },
                err => {
                    this.showError(err);
                }
            )
            .then(() => {
                this.setState({ updating: false });
            });
    }

    render() {
        const { bin } = this.props;
        const { popoverOpen, updating, error } = this.state;

        const binColor = binColors[bin.type];
        const bgColor = backgroundColors[bin.type];

        return (
            <div>

                {/* Bin icon */}
                <div className="fa-5x" id={"bin-marker-" + bin._id}
                    style={{ cursor: 'pointer', display: 'inline-block' }} onClick={this.toggle}>
                    <span className="fa-layers fa-fw">

                        {/* Bubble */}
                        <FAIcon icon="circle" color={bgColor} />
                        <FAIcon icon="square" color={bgColor} transform="shrink-6 right-3.1 down-3.1" />

                        {/* Bin */}
                        {!updating && <FAIcon icon={[ 'far', 'trash-alt' ]} color={binColor} transform="shrink-6" />}

                        {/* Spinner */ }
                        {updating && <FAIcon icon="cog" color={binColor} size="xs" spin />}

                    </span>
                </div>

                <Popover placement="top" isOpen={popoverOpen} target={"bin-marker-" + bin._id} toggle={this.toggle}>
                    <PopoverHeader>{bin.address}</PopoverHeader>
                    <PopoverBody>
                        
                        {/* Stats */}
                        <BinStats bin={bin} isIndeterminate={updating} style={{ marginBottom: '5px' }} />

                        {/* Error message */}
                        {error &&
                            <div style={{ marginBottom: '5px', color: 'red', fontWeight: 'bold' }}>{error}</div>
                        }

                        {/* Update button */}
                        <div className="text-right">
                            <Button color="primary" size="sm" onClick={this.onUpdateClick} disabled={updating}>Update</Button>
                        </div>

                    </PopoverBody>
                </Popover>
            </div>
        );
    }
}