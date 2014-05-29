#!/usr/bin/python

import time

# These next two imports are common POX convention
from pox.lib.revent import *
from pox.core import core
import pox.openflow.libopenflow_01 as of
from pox.lib.util import dpidToStr, str_to_dpid
import pox.lib.packet as pkt
import pox.lib.packet.arp as arp
import pox.lib.packet.ethernet as ethernet

from pox.lib.addresses import EthAddr

from pox.lib.recoco import Timer

# include as part of the betta branch
from pox.openflow.of_json import *

from collections import namedtuple,defaultdict

# from twowaydict import TwoWayDict

# Even a simple usage of the logger is much nicer than print!
log = core.getLogger()


# Dict of switches adjacent to mac add: [mac] -> Switch
# mac_adjacency = TwoWayDict(lambda:None)




# Component install 3 hardcoded data paths between 4 switches
#
#

class MPHardSwitch(EventMixin):
    """
    Component installs hardcoded data paths based on hw addresses
    """

    _core_name = "mphardswitch" # Module name: core.link_monitor

    def __init__(self):
        core.openflow_discovery.addListeners(self)
        core.openflow.addListeners(self)

        self.brid = {}
        self.connections = defaultdict(lambda:None)
        for i in range(1,5):
            self.brid['br{}'.format(i)] = str_to_dpid("fe-fe-00-0a-ff-0{}".format(i))

        # Dict of dicts. [sw1][sw2] return port of sw1 connected to sw2
        # Also used for port location of adjacent mac addrs:
        # [sw][mac] -> port of switch connected to mac
        # self.switch_adjacency = defaultdict(lambda:defaultdict(lambda:None))

        # self.table = dict()

        self.switches = dict()
        # self.ip_mac = TwoWayDict()

    ## Listener for core.openflow_discovery
    def _handle_LinkEvent(self, event):
        link = event.link # The link tuple (attributes dpid1, port1, dpid2, port2


        if event.added:
            print("Link added: {}.{} <-> {}.{}".format(dpidToStr(link.dpid1),
                                                       link.port1,
                                                       dpidToStr(link.dpid2),
                                                       link.port2))

            ## Install hardcoded flows
            ## While we're at it, disable flooding on sidepaths
            hwsrc = None
            # hwdst = None
            if link.dpid1 == self.brid['br1']:
                if link.dpid2 == self.brid['br2']:
                    hwsrc = EthAddr("fe:fe:00:0a:01:02")
                    self.disable_flood(link.dpid1,link.port1)
                elif link.dpid2 == self.brid['br3']:
                    hwsrc = EthAddr("fe:fe:00:0a:01:03")
                    self.disable_flood(link.dpid1,link.port1)
                elif link.dpid2 == self.brid['br4']:
                    hwsrc = EthAddr("fe:fe:00:0a:01:01")
                    self.disable_flood(link.dpid1,link.port1)
                else:
                    log.warning("Link endpoint is not recognised:"+
                                dpidToStr(link.dpid2))

            elif link.dpid1 == self.brid['br2']:
                if link.dpid2 == self.brid['br1']:
                    hwsrc = EthAddr("fe:fe:00:0a:02:02")
                elif link.dpid2 == self.brid['br4']:
                    hwsrc = EthAddr("fe:fe:00:0a:01:02")
                else:
                    log.warning("Link endpoint is not recognised:"+
                                dpidToStr(link.dpid2))

            elif link.dpid1 == self.brid['br3']:
                if link.dpid2 == self.brid['br1']:
                    hwsrc = EthAddr("fe:fe:00:0a:02:03")
                elif link.dpid2 == self.brid['br4']:
                    hwsrc = EthAddr("fe:fe:00:0a:01:03")
                else:
                    log.warning("Link endpoint is not recognised:"+
                                dpidToStr(link.dpid2))

            elif link.dpid1 == self.brid['br4']:
                if link.dpid2 == self.brid['br2']:
                    hwsrc = EthAddr("fe:fe:00:0a:02:02")
                    self.disable_flood(link.dpid1,link.port1)
                elif link.dpid2 == self.brid['br3']:
                    hwsrc = EthAddr("fe:fe:00:0a:02:03")
                    self.disable_flood(link.dpid1,link.port1)
                elif link.dpid2 == self.brid['br1']:
                    hwsrc = EthAddr("fe:fe:00:0a:02:01")
                    self.disable_flood(link.dpid1,link.port1)
                else:
                    log.warning("Link endpoint is not recognised:"+
                                dpidToStr(link.dpid2))
            else:
                log.warning("Link start-point is not recognised:"+
                            dpidToStr(link.dpid1))
            if hwsrc: self.install_flow(link.dpid1, link.port1, src=hwsrc)
            else: log.warning("No src hw address resolved!")


        elif event.removed:
            log.debug("Link removed: {}.{} <-> {}.{}".format(link.dpid1,
                                                          link.port1,
                                                          link.dpid2,
                                                          link.port2))
        else:
            assert(0) # LinkEvent acting up


    def _handle_ConnectionUp(self, event):
        print("Switch {} has come up.".format(dpidToStr(event.dpid)))
        self.switches[event.dpid] = event.connection

    def _handle_ConnectionDown(self, event):
        log.debug("Switch %s has gone down.", dpidToStr(event.dpid))
        del self.switches[event.dpid]

    def _handle_PacketIn(self, event):
        packet = event.parsed

        if packet.dst.is_multicast:
                return

        # Learn the source and fill up routing table
        # self.table[(event.connection,packet.src)] = event.port
        # dst_port = self.table.get((event.connection,packet.dst))

        print("Switch {} has no flow for dst {} or src {}".format(
            dpidToStr(event.dpid),packet.dst,packet.src))

        # setup reverse flow:
        # "Every packet with dst=packet.src should go out event.port of
        # event.connection"

        # arp = packet.find('arp')

        dst_port = self.find_edge_port(event.dpid)

        if not dst_port:
            log.warning("Switch {} could not find edge port".format(
                dpidToStr(event.dpid)))
            return

        self.install_flow(event.dpid, dst_port, src=packet.src)

        self.send_packet(event, dst_port)

        # if dst_port is None:
            # We don't know where the destination is yet. So, we'll just
            # send the packet out all ports (except the one it came in on!)
            # and hope the destination is out there somewhere. :)
            # To send out all ports, we can use either of the special ports
            # OFPP_FLOOD or OFPP_ALL. We'd like to just use OFPP_FLOOD,
            # but it's not clear if all switches support this. :(
            # self.send_packet(event, of.)

            # print("Switch {} Flooding packet from {} at port {} with dst {}"
            #       "".format(dpidToStr(event.dpid), packet.src, event.ofp.in_port,
            #                 packet.dst, of.OFPP_FLOOD))



    # Method for just sending a packet to any port (flood by default)
    def send_packet(self, event, dst_port = of.OFPP_FLOOD):
        msg = of.ofp_packet_out(in_port=event.ofp.in_port)
        if event.ofp.buffer_id != -1 and event.ofp.buffer_id is not None:
            # We got a buffer ID from the switch; use that
            msg.buffer_id = event.ofp.buffer_id
        else:
            # No buffer ID from switch -- we got the raw data
            if event.ofp.data:
                # No raw_data specified -- nothing to send!
                return
            msg.data = event.ofp.data
        msg.actions.append(of.ofp_action_output(port = dst_port))
        event.connection.send(msg)

    def find_edge_port(self, dpid):
        """
        Uses the openflow.discovery module to find port that is not connected
        to other switches. Return port number of first found edge port.
        NB: This is not pretty, and is only remotely useable for a
        hardcoded switch (like this), where we know only 1 edge port exist.
        """
        # Iterate over ports:
        conn = self.switches[dpid]
        for p in conn.ports.itervalues():
            if core.openflow_discovery.is_edge_port(dpid, p.port_no):
                return p.port_no
        else:
            return None


    def disable_flood(self, dpid, port_no):
        log.debug("Disabling flooding for port {} on switch {}".format(
            port_no, dpidToStr(dpid)))

        conn = self.switches[dpid]
        for p in conn.ports.itervalues():
            if p.port_no == port_no:
                msg = of.ofp_port_mod(port_no=p.port_no,
                                      hw_addr=p.hw_addr,
                                      config=of.OFPPC_NO_FLOOD,
                                      mask=of.OFPPC_NO_FLOOD)
                conn.send(msg)
                break
        else:
            log.warning("Could not find port to disable flooding")

    # def respond_to_arp(self, packet, match, event):
    #     # reply to ARP request

    #     # Check if we know the associated mac for the requested IP.
    #     requested_mac = self.ip_mac.get(match.nw_dst)
    #     if not requested_mac:
    #         # nothing to do, skip arp reply for now
    #         log.info("ARP request unresolved.")
    #         return

    #     reply = arp()
    #     reply.opcode = arp.REPLY
    #     reply.protosrc = match.nw_dst
    #     reply.protodst = match.nw_src

    #     reply.hwsrc = requested_mac
    #     reply.hwdst = match.dl_src

    #     eth = ethernet(type=packet.ARP_TYPE, src=reply.hwsrc, dst=reply.hwdst)
    #     eth.set_payload(reply)
    #     log.debug("{0} {1} answering ARP for {2}".format(event.dpid,
    #                                                      event.port,
    #                                                      str(reply.protosrc)))
    #     msg = of.ofp_packet_out()
    #     msg.data = eth.pack()
    #     msg.actions.append(of.ofp_action_output(port =
    #                                             of.OFPP_IN_PORT))
    #     msg.in_port = event.port
    #     event.connection.send(msg)

    def install_flow(self, dpid, out_port, src=None, dst=None, in_port=None, buf=None):
        # At least one of src, dst, in_port must be set:
        assert(src or dst or in_port)
        print("Installing flow from {0} (p{1}) to {2} (p{3}) on switch {4}".format(
            src,in_port,dst,out_port,dpidToStr(dpid)))
        msg = of.ofp_flow_mod()
        if in_port: msg.match.in_port = in_port
        if src: msg.match.dl_src = src
        if dst: msg.match.dl_dst = dst
        msg.idle_timeout = 0
        msg.hard_timeout = 0
        msg.actions.append(of.ofp_action_output(port = out_port))
        if buf: msg.buffer_id = buf
        self.switches[dpid].send(msg)


# Launch function. Called by POX on launch.
def launch ():
    core.registerNew(MPHardSwitch)
    # core.link_monitor.addListener(PacketLossEvent, test_PacketLossEvent)

    log.info(" MPHardSwitch active.")
