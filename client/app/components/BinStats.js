import React from 'react'; // eslint-disable-line
import { Progress } from 'reactstrap';

export default ({ bin, isIndeterminate = false, ...rest }) => {
    
    let level = (bin.height - (bin.lastMeasurement ? bin.lastMeasurement.value : bin.height)) / bin.height;
    level = level < 0 ? 0 : (level > 1 ? 1 : level);

    const progressColor = isIndeterminate ? 'info' : level <= 1 / 3 ? 'success' : level <= 2 / 3 ? 'warning': 'danger';

    return (
        <div {...rest}>
            Type: <strong>{bin.type[0].toUpperCase() + bin.type.substring(1)}</strong><br />
            Full: <strong>{(level * 100).toFixed(2)}%</strong><br />
            <Progress value={isIndeterminate ? 100 : level * 100} color={progressColor} animated={isIndeterminate} />
        </div>
    );
};