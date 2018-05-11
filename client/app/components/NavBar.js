import React from 'react';
import {
    Collapse,
    Navbar,
    NavbarToggler,
    NavbarBrand,
    Nav,
    NavItem,
    NavLink,
    UncontrolledDropdown,
    DropdownToggle,
    DropdownItem,
    DropdownMenu
} from 'reactstrap';

export default class NavBar extends React.Component {

    constructor(props) {
        super(props);

        this.toggle = this.toggle.bind(this);
        this.state = {
            isOpen: false
        };
    }

    toggle() {
        this.setState({
            isOpen: !this.state.isOpen
        });
    }

    render() {
        const { computeRouteForBins } = this.props;
        return (
            <Navbar color="inverse" light expand="md">
                <NavbarBrand href="/">Internet of Trash</NavbarBrand>
                <NavbarToggler onClick={this.toggle} />
                <Collapse isOpen={this.state.isOpen} navbar>
                    <Nav className="ml-auto" navbar>
                        <UncontrolledDropdown nav inNavbar>
                            <DropdownToggle nav caret>
                                Prepare route
                            </DropdownToggle>
                            <DropdownMenu right>
                                <DropdownItem onClick={computeRouteForBins.bind(null, 'plastic')}>
                                    Plastic
                                </DropdownItem>
                                <DropdownItem onClick={computeRouteForBins.bind(null, 'paper')}>
                                    Paper
                                </DropdownItem>
                                <DropdownItem onClick={computeRouteForBins.bind(null, 'organic')}>
                                    Organic
                                </DropdownItem>
                                <DropdownItem onClick={computeRouteForBins.bind(null, 'generic')}>
                                    Generic
                                </DropdownItem>
                                <DropdownItem divider />
                                <DropdownItem onClick={computeRouteForBins.bind(null, 'all')}>
                                    All
                                </DropdownItem>
                                <DropdownItem divider />
                                <DropdownItem onClick={computeRouteForBins.bind(null, null)}>
                                    Clear
                                </DropdownItem>
                            </DropdownMenu>
                        </UncontrolledDropdown>
                        <NavItem>
                            <NavLink href="https://github.com/95ulisse/InternetOfTrash">GitHub</NavLink>
                        </NavItem>
                    </Nav>
                </Collapse>
            </Navbar>
        );
    }

}