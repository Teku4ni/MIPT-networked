#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include <stdlib.h>
#include <vector>
#include <map>
#include <cmath>

static std::vector<Entity> entities;
static std::map<uint16_t, ENetPeer*> controlledMap;
static std::vector<std::pair<float, float>> aiObjectives;
static std::vector<bool> collisions;
static int aiEntitiesNumber = 15;

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
  // send all entities
  for (const Entity &ent : entities)
    send_new_entity(peer, ent);

  // find max eid
  uint16_t maxEid = entities.empty() ? invalid_entity : entities[0].eid;
  for (const Entity &e : entities)
    maxEid = std::max(maxEid, e.eid);
  uint16_t newEid = maxEid + 1;
  uint32_t color = 0x000000ff +
                   ((rand() % 256) << 8) +
                   ((rand() % 256) << 16) +
                   ((rand() % 256) << 24);
  float x = -350 + (rand() % 700);
  float y = -350 + (rand() % 700);
  float radius = 20 + rand() % 20;
  Entity ent = {color, x, y, radius, newEid};
  entities.push_back(ent);

  controlledMap[newEid] = peer;


  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, newEid);
}

void on_state(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f;
  deserialize_entity_state(packet, eid, x, y);
  for (Entity &e : entities)
    if (e.eid == eid)
    {
      e.x = x;
      e.y = y;
    }
}

void generate_ai_entities()
{
  for (uint16_t i = 0; i < aiEntitiesNumber; ++i)
  {
    uint32_t color = 0x000000ff +
                     ((rand() % 256) << 8) +
                     ((rand() % 256) << 16) +
                     ((rand() % 256) << 24);
    float x = -350 + (rand() % 700);
    float y = -350 + (rand() % 700);
    float radius = 20 + rand() % 20;
    Entity ent = {color, x, y, radius, i};

    entities.push_back(ent);
    aiObjectives.emplace_back(-350 + (rand() % 700), -350 + (rand() % 700));
  }
}

void move_ai_entities()
{
  float step = 1.f;
  for (uint16_t i = 0; i < aiEntitiesNumber; ++i)
  {
    float x_distance = aiObjectives[i].first - entities[i].x;
    float y_distance = aiObjectives[i].second - entities[i].y;
    float distance = std::sqrt(x_distance * x_distance + y_distance * y_distance);
    if (distance < step / 2)
      aiObjectives[i] = {-350 + (rand() % 700), -350 + (rand() % 700)};
    else
    {
      entities[i].x += step * x_distance / distance;
      entities[i].y += step * y_distance / distance;
    }
  }
}

float dist2(const Entity& a, const Entity& b) 
{
  return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
};

void resolve_collisions()
{
  collisions.assign(entities.size(), false);

  for (Entity& e1 : entities)
  {
    for (Entity& e2 : entities)
    {
      if (e1.radius < e2.radius && dist2(e1, e2) < (e1.radius + e2.radius) * (e1.radius + e2.radius))
      {
        e2.radius = std::sqrt(e2.radius * e2.radius + e1.radius * e1.radius / 2.f);
        collisions[e2.eid] = true;

        e1.x = -350 + (rand() % 700);
        e1.y = -350 + (rand() % 700);
        e1.radius /= std::sqrt(2);
        collisions[e1.eid] = true;
      }
    }
  }
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10131;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  generate_ai_entities();

  while (true)
  {
    ENetEvent event;
    while (enet_host_service(server, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
          case E_CLIENT_TO_SERVER_JOIN:
            on_join(event.packet, event.peer, server);
            break;
          case E_CLIENT_TO_SERVER_STATE:
            on_state(event.packet);
            break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    move_ai_entities();
    resolve_collisions();

    static int t = 0;
    for (const Entity &e : entities)
      for (size_t i = 0; i < server->peerCount; ++i)
      {
        ENetPeer *peer = &server->peers[i];
        if (collisions[e.eid] || controlledMap[e.eid] != peer)
          send_snapshot(peer, e.eid, e.x, e.y, e.radius);
      }
    Sleep(10);
  }

  enet_host_destroy(server);
  
  atexit(enet_deinitialize);
  return 0;
}
