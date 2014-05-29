In order to run set virtual setup configurations, some dependencies must be met. 

The following dependencies must be installed:

	tmux
	kvm
	openvswitch-switch

The versions shipped with ubuntu 14.04 package manager (apt-get) has been used.

Apart from this, the ubuntu image files must be fetched. This can be done with the following terminal commands

Download:
	wget https://dl.dropboxusercontent.com/u/2347095/virtuals.tar.bz2 

Unpack:
	tar -xvf virtuals.tar.bz2

The virtual images are 64-bit, requiring the host OS to be 64 bit as well.

Furthermore, the ip_forward script must be included in the path:
	
	cp ip_forward /usr/local/bin/ip_forward
	chmod +x /usr/local/bin/ip_forward

Alternatively, the "ip_forward" call in the setup scripts can be changed to "./ip_forward".


It is strongly recommended to setup ssh keys for password-less ssh access to the virtual nodes.

If the host OS does not have a public ssh key, generate a key pair with
	ssh-keygen -t rsa

then, launch the base virtual machine inside a screen or tmux instance with the command
	./base.sh
or
	./base_mptcp.sh
if the mptcp capable virtual machine is to be used.

To copy the public key to the node, use
	ssh-copy-id [-i keyfile] node@10.0.2.1
The ssh-copy-id will choose the default public key if no keyfile is specified.


Hostnames can be setup on the host OS for quick access to the nodes:

Add the following lines to /etc/hosts (for ubuntu host OS at least)

	10.0.2.1	n1
	10.0.2.2	n2
After this, the host should be able to access node1 through ssh with
	ssh node@n1
and node2 with
	ssh node@n2


The "share" parameter in the setup scripts needs to correspond with the coding location in the runner scripts. The "share" location is mounted in ~/share directory in the virtual machines (requires sudo mount hostshare call in the guest OS's, but this is called in the setup scripts). The coding can also be fetched and built directly on the virtual machines. If this method is preffered, fetch and build the code inside the base machines (by launching the virtual machine with "./base.sh" or "./base_mptcp.sh") for the changes to persist.
Changes made to the virtual machines launched with the remaining setup scripts does not persist between launches.


