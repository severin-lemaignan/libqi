/*
**  Copyright (C) 2012 Aldebaran Robotics
**  See COPYING for the license
*/

// Disable "'this': used in base member initializer list"
#ifdef _MSC_VER
# pragma warning( push )
# pragma warning(disable: 4355)
#endif

#include <qi/log.hpp>

#include "transportsocket.hpp"
#include "tcptransportsocket.hpp"

qiLogCategory("qimessaging.transportsocket");

namespace qi
{



  TransportSocket::~TransportSocket()
  {
    qiLogDebug() << "Destroying transport socket";
  }

  TransportSocketPtr makeTransportSocket(const std::string &protocol, qi::EventLoop *eventLoop) {
    TransportSocketPtr ret;

    if (protocol == "tcp")
    {
      return TcpTransportSocketPtr(new TcpTransportSocket(eventLoop, false));
    }
    else if (protocol == "tcps")
    {
      return TcpTransportSocketPtr(new TcpTransportSocket(eventLoop, true));
    }
    else
    {
      qiLogError() << "Unrecognized protocol to create the TransportSocket: " << protocol;
      return ret;
    }
  }


}

#ifdef _MSC_VER
# pragma warning( pop )
#endif
