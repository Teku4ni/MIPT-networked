#include "protocol.h"
#include <cstring> // memcpy
#include "bitstream.h"

void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
  *packet->data = E_CLIENT_TO_SERVER_JOIN;

  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(Entity),
                                                   ENET_PACKET_FLAG_RELIABLE);
  Bitstream bs(packet->data);
  bs.write(E_SERVER_TO_CLIENT_NEW_ENTITY);
  bs.write(ent);

  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t),
                                                   ENET_PACKET_FLAG_RELIABLE);
  Bitstream bs(packet->data);
  bs.write(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY);
  bs.write(eid);

  enet_peer_send(peer, 0, packet);
}

void send_entity_input(ENetPeer *peer, uint16_t eid, float thr, float steer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   2 * sizeof(float),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  Bitstream bs(packet->data);
  bs.write(E_CLIENT_TO_SERVER_INPUT);
  bs.write(eid);
  bs.write(thr);
  bs.write(steer);

  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float ori, uint32_t timestamp)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   3 * sizeof(float) + sizeof(uint32_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  Bitstream bs(packet->data);
  bs.write(E_SERVER_TO_CLIENT_SNAPSHOT);
  bs.write(eid);
  bs.write(x);
  bs.write(y);
  bs.write(ori);
  bs.write(timestamp);

  enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  MessageType msg;
  Bitstream bs(packet->data);
  bs.read(msg);
  bs.read(ent);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  MessageType msg;
  Bitstream bs(packet->data);
  bs.read(msg);
  bs.read(eid);
}

void deserialize_entity_input(ENetPacket *packet, uint16_t &eid, float &thr, float &steer)
{
  MessageType msg;
  Bitstream bs(packet->data);
  bs.read(msg);
  bs.read(eid);
  bs.read(thr);
  bs.read(steer);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &ori, uint32_t &timestamp)
{
  MessageType msg;
  Bitstream bs(packet->data);
  bs.read(msg);
  bs.read(eid);
  bs.read(x);
  bs.read(y);
  bs.read(ori);
  bs.read(timestamp);
}
