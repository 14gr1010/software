The project requires some dependencies.
One of the dependencies is Boost, and to install it on Debian/Ubuntu run (X.YZ >= 1.49):
    sudo apt-get install libboostX.YZ-dev

To configure waf and download additional dependencies from GitHub:
    mkdir -p ../dependencies
    ./waf configure --bundle=ALL --bundle-path=../dependencies/ --options=cxx_debug

To build all executables:
    ./waf

To run the test (requires a tun interface, see below), run the executable in:
    ./build/test

For the coded tunnels (requires a tun interface, see below) run the executables in either (depending on the setup):
    ./build/apps/singlepath
    ./build/apps/multipath

The coded tunnel executables require a various command line parameters, and by running the executable without parameters shows the order in which they must be provided.

Create a TUN interface, in this case we create tun0 with ip 10.0.0.1:
    ./scripts/tun_setup.sh tun0 10.0.0.1

Some tests are known to fail on low performance setups.
