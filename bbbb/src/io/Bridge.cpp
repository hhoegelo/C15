#include <io/Bridge.h>
#include "Receiver.h"
#include "Sender.h"
#include <functional>

Bridge::Bridge(Sender *sender, Receiver *receiver)
    : m_sender(sender)
    , m_receiver(receiver)
{
  m_receiver->setCallback(std::bind(&Bridge::transmit, this, std::placeholders::_1));
}

Bridge::~Bridge()
{
}

void Bridge::transmit(Receiver::tMessage msg)
{
  m_sender->send(msg);
}
