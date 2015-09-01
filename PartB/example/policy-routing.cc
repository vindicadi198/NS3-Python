#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-helper.h"
#include "ns3/applications-module.h"
#include <iostream>

using namespace ns3;
using namespace std;
Ipv6InterfaceContainer i1,i2,i3;

// Callback for the node running the TCP Client 's IPv6 stack
Ptr<Ipv6Route> 
callback1(const TcpHeader* tcpHeader, const Ipv6Header* ipHeader, void* user_data)
{
  cout<<"Node1:Intercepted packet headed from port "<<tcpHeader->GetSourcePort()<<" to "<<tcpHeader->GetDestinationPort();
  // Divert all TCP Flows to the other NetDevice after 10 seconds
  if(Simulator::Now().GetSeconds()>10)
  {
    cout<<" and sending via new interface "<<endl;
    Ptr<Ipv6Route> route(new Ipv6Route());
    route->SetOutputDevice(((Ipv6*)user_data)->GetNetDevice(2));
    route->SetGateway(i3.GetAddress(1,1));
    return route;
  }
  cout<<endl;
  return NULL;
}

// Callback for the node running the TCP server
Ptr<Ipv6Route> 
callback2(const TcpHeader* tcpHeader, const Ipv6Header* ipHeader, void* user_data)
{
  cout<<"Node2:Intercepted packet headed from port "<<tcpHeader->GetSourcePort()<<" to "<<tcpHeader->GetDestinationPort();
  // Divert all TCP Flows to the other NetDevice after 10 seconds
  if(Simulator::Now().GetSeconds()>10)
  {
    cout<<" and sending via new interface "<<endl;
    Ptr<Ipv6Route> route(new Ipv6Route());
    route->SetOutputDevice(((Ipv6*)user_data)->GetNetDevice(2));
    route->SetGateway(i3.GetAddress(0,1));
    return route;
  }
  cout<<endl;
  return NULL;
}

int 
main (int argc, char **argv)
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  /* Create 3 nodes */
  NodeContainer n;
  n.Create (3);

  /* Install the IP stack */
  InternetStackHelper internetv6;
  internetv6.SetIpv4StackInstall (false);
  internetv6.Install (n);

  /* Create the CSMA Helper */
  CsmaHelper csmaHelper;
  csmaHelper.SetChannelAttribute("DataRate",StringValue("8Kbps"));
  Ipv6AddressHelper ipv6(Ipv6Address("2013::"),Ipv6Prefix(64));

  /* Add CSMA channel and NetDevice for each pair of nodes */
  NodeContainer n1;
  n1.Add(n.Get(0));
  n1.Add(n.Get(1));
  NetDeviceContainer d1 = csmaHelper.Install (n1);
  ipv6.NewNetwork();
  i1 = ipv6.Assign (d1);

  NodeContainer n2;
  n2.Add(n.Get(1));
  n2.Add(n.Get(2));
  NetDeviceContainer d2 = csmaHelper.Install (n2);
  ipv6.NewNetwork();
  i2 = ipv6.Assign (d2);

  NodeContainer n3;
  n3.Add(n.Get(0));
  n3.Add(n.Get(2));
  NetDeviceContainer d3 = csmaHelper.Install (n3);
  ipv6.NewNetwork();
  i3 = ipv6.Assign (d3);

  /* Set up IP Forwarding on all the Ipv6 stacks in the nodes */
  Ptr<Ipv6> node0IpStack=n.Get(0)->GetObject<Ipv6>();
  node0IpStack->SetForwarding(1,true);
  node0IpStack->SetForwarding(2,true);

  Ptr<Ipv6> node1IpStack=n.Get(1)->GetObject<Ipv6>();
  node1IpStack->SetForwarding(1,true);
  node1IpStack->SetForwarding(2,true);

  Ptr<Ipv6> node2IpStack=n.Get(2)->GetObject<Ipv6>();
  node2IpStack->SetForwarding(1,true);
  node2IpStack->SetForwarding(2,true);

  /* Set up Ipv6 static routing to reach the other nodes */
  Ipv6StaticRoutingHelper helper;
  Ptr<Ipv6StaticRouting> route0 = helper.GetStaticRouting(node0IpStack);
  route0->AddHostRouteTo(i2.GetAddress(1,1),i1.GetAddress(1,1),1);
  route0->SetRouteCallback(callback1,PeekPointer(node0IpStack));

  Ptr<Ipv6StaticRouting> route2 = helper.GetStaticRouting(node2IpStack);
  route2->AddHostRouteTo(i1.GetAddress(0,1),i2.GetAddress(0,1),1);
  route2->SetRouteCallback(callback2,PeekPointer(node2IpStack));

  /* Create the TCP BulkSendHelper and PacketSinkHelper objects */
  Inet6SocketAddress destination=Inet6SocketAddress(Ipv6Address(i2.GetAddress(1,1)),5001);
  BulkSendHelper sender("ns3::TcpSocketFactory",destination);
  sender.SetAttribute("MaxBytes", UintegerValue(10 * 1024));
  PacketSinkHelper sink("ns3::TcpSocketFactory",destination);

  /* Install the applications and set their start and end times */
  ApplicationContainer client = sender.Install (n.Get (0));
  client.Start (Seconds (2.0));
  client.Stop (Seconds (180.0));

  ApplicationContainer server = sink.Install (n.Get (2));
  server.Start (Seconds (2.0));
  server.Stop (Seconds (180.0));

  Simulator::Run ();
  Simulator::Destroy ();

}

