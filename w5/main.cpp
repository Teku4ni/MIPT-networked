// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <functional>
#include "raylib.h"
#include <enet/enet.h>
#include <math.h>
#include <algorithm> 

#include <vector>
#include <map>
#include <deque>
#include "entity.h"
#include "protocol.h"

struct Snapshot
{
  float x;
  float y;
  float ori;
  uint32_t timestamp;
};

struct Input
{
  float thr;
  float steer;
  uint32_t timestamp; 
};

static std::map<uint16_t, Entity> entities;
static uint16_t my_entity = invalid_entity;
std::unordered_map<uint16_t, std::deque<Snapshot>> snapshots = {};
std::deque<Snapshot> myEntitySnapshots;
std::deque<Input> inputs;

void simulate(float thr, float steer, uint32_t ticks) 
{
  Entity &e = entities[my_entity];
  e.thr = thr;
  e.steer = steer;
  for (uint32_t t = 0; t < ticks; ++t)
  {
    simulate_entity(e, 0.01f);
    e.timestamp++;
    myEntitySnapshots.emplace_back(e.x, e.y, e.ori, e.timestamp);
  }
}

void interpolate(Entity& entity, uint32_t curTime)
{
  if (snapshots[entity.eid].size() < 2)
    return;
  auto snapshotTime = snapshots[entity.eid][1].timestamp;
  while (curTime > snapshotTime && snapshots[entity.eid].size() > 2) 
  {
    snapshots[entity.eid].pop_front();
    snapshotTime = snapshots[entity.eid][1].timestamp;
  }

  Snapshot& curSnapshot = snapshots[entity.eid][1];
  Snapshot& prevSnapshot = snapshots[entity.eid][0];
  float t = float(curTime - prevSnapshot.timestamp) / float(curSnapshot.timestamp - prevSnapshot.timestamp);
  entity.x = prevSnapshot.x + t * (curSnapshot.x - prevSnapshot.x);
  entity.y = prevSnapshot.y + t * (curSnapshot.y - prevSnapshot.y);
  entity.ori = prevSnapshot.ori + t * (curSnapshot.ori - prevSnapshot.ori);
}

void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntity;
  deserialize_new_entity(packet, newEntity);
  if (!entities.contains(newEntity.eid) && newEntity.eid != my_entity)
  {
    entities[newEntity.eid] = std::move(newEntity);
    snapshots[newEntity.eid].push_back({newEntity.x, newEntity.y, newEntity.ori, enet_time_get()});
  }
}

void on_set_controlled_entity(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_entity);
}

void on_snapshot(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f; float ori = 0.f;
  uint32_t curTimestamp = 0;
  deserialize_snapshot(packet, eid, x, y, ori, curTimestamp);
  if (eid != my_entity)
    snapshots[eid].emplace_back(x, y, ori, enet_time_get() + 150);
  else
  {
    while (!myEntitySnapshots.empty())
    {
      Snapshot &snap = myEntitySnapshots.front();
      if (snap.timestamp > curTimestamp)
        break;
      if (snap.timestamp == curTimestamp && (snap.x != x || snap.y != y || snap.ori != ori))
      {
        entities[my_entity].x = x;
        entities[my_entity].y = y;
        entities[my_entity].ori = ori;

        while (inputs.size() > 0 && inputs.front().timestamp < curTimestamp)
          inputs.pop_front();
        size_t n = inputs.size();
        for (size_t i = 0; (i < n - 1 && inputs[i].timestamp < curTimestamp); ++i)
          simulate(inputs[i].thr, inputs[i].steer, inputs[i + 1].timestamp - inputs[i].timestamp);
        simulate(inputs[n - 1].thr, inputs[n - 1].steer, curTimestamp - inputs[n - 1].timestamp);
        entities[my_entity].timestamp = curTimestamp;

        myEntitySnapshots.clear();
        break;
      }
      myEntitySnapshots.pop_front();
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

  ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "localhost");
  address.port = 10131;

  ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
  if (!serverPeer)
  {
    printf("Cannot connect to server");
    return 1;
  }

  int width = 600;
  int height = 600;

  InitWindow(width, height, "w5 networked MIPT");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight - 150, height);
    SetWindowSize(width, height);
  }

  Camera2D camera = { {0, 0}, {0, 0}, 0.f, 1.f };
  camera.target = Vector2{ 0.f, 0.f };
  camera.offset = Vector2{ width * 0.5f, height * 0.5f };
  camera.rotation = 0.f;
  camera.zoom = 10.f;


  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
  bool connected = false;
  uint32_t prevTime = enet_time_get();
  while (!WindowShouldClose())
  {
    uint32_t curTime = enet_time_get();
    float dt = GetFrameTime();
    ENetEvent event;
    while (enet_host_service(client, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        send_join(serverPeer);
        connected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
        case E_SERVER_TO_CLIENT_NEW_ENTITY:
          on_new_entity_packet(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
          on_set_controlled_entity(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SNAPSHOT:
          on_snapshot(event.packet);
          break;
        };
        break;
      default:
        break;
      };
    }
    if (my_entity != invalid_entity)
    {
      bool left = IsKeyDown(KEY_LEFT);
      bool right = IsKeyDown(KEY_RIGHT);
      bool up = IsKeyDown(KEY_UP);
      bool down = IsKeyDown(KEY_DOWN);
      // TODO: Direct adressing, of course!
      if (entities.contains(my_entity))
      {
        // Update
        float thr = (up ? 1.f : 0.f) + (down ? -1.f : 0.f);
        float steer = (left ? -1.f : 0.f) + (right ? 1.f : 0.f);
        uint32_t ts = entities[my_entity].timestamp;
        simulate(thr, steer, (curTime - prevTime) / 10);
        prevTime += (curTime - prevTime) / 10 * 10;
        if (ts != entities[my_entity].timestamp)
        {
          inputs.push_back(Input{thr, steer, ts});
          // Send
          send_entity_input(serverPeer, my_entity, thr, steer);
        }
      }
      for (auto& e : entities)
        if (e.second.eid != my_entity)
          interpolate(e.second, curTime);
    }
    BeginDrawing();
      ClearBackground(WHITE);
      BeginMode2D(camera);
        for (const auto &e : entities)
        {
          const Rectangle rect = {e.second.x, e.second.y, 3.f, 1.f};
          DrawRectanglePro(rect, {0.f, 0.5f}, e.second.ori * 180.f / PI, GetColor(e.second.color));
        }

      EndMode2D();
    EndDrawing();
  }

  CloseWindow();
  return 0;
}