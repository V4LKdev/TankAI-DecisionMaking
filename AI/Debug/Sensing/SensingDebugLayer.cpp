#include "AI/Debug/Sensing/SensingDebugLayer.h"
#include "AI/AISubsystem.h"
#include "AI/Gateway/AIServiceGateway.h"
#include "CoreTank/Tank.h"
#include "Globals.h"
#include "Services/Sensing/Audio/Bus.h"
#include "Services/Sensing/Sensing.h"
#include <Play.h>
#include <algorithm>
#include <cmath>
#include <ranges>
#include <unordered_set>

namespace AI {

namespace {
  // Draw a cone by densely drawing radial lines from center (looks cool)
  // Also draw the two boundary sight lines for visual clarity
  void drawFilledCone(const Play::Vector2D& center, const float radius,const float a0, const float a1) {

    constexpr Play::Colour fillCol{ 0, 200, 255, 50 }; // subtle cyan
    constexpr Play::Colour edgeCol = Play::cWhite;

    const Play::Vector2D e0 { center.x + std::cosf(a0) * radius, center.y + std::sinf(a0) * radius };
    const Play::Vector2D e1 { center.x + std::cosf(a1) * radius, center.y + std::sinf(a1) * radius };
    Play::DrawLine(center, e0, edgeCol);
    Play::DrawLine(center, e1, edgeCol);

    constexpr int segments = 72;
    const float da = (a1 - a0) / static_cast<float>(segments);
    for (int i = 0; i <= segments; ++i) {
      const float a = a0 + da * static_cast<float>(i);
      const Play::Vector2D p { center.x + std::cosf(a) * radius, center.y + std::sinf(a) * radius };
      Play::DrawLine(center, p, fillCol);
    }
  }

  uint8_t u8(const float v) { return static_cast<uint8_t>(std::clamp(v, 0.0f, 255.0f)); }
  float clamp01(const float v) { return std::clamp(v, 0.0f, 1.0f); }
}

// Sub-layer visibility toggles
static bool showVision_ = true;
static bool showAudio_  = true;

void SensingDebugLayer::render(const AISubsystem& sys) const {
  if (!visible_) return;

  // HUD scratch setup
  hudLines_.clear(); hudLines_.reserve(8);

  for (const auto& [id, agent] : sys.getAgents()) {
    if (!agent.tank || !agent.gw || !agent.sensing) continue;

    const auto& tank = *agent.tank;
    const auto& gw   = *agent.gw;
    const auto& sens = *agent.sensing;

    const Play::Vector2D pos = tank.GetPosition();
    const float rot = tank.GetRotation();
    const auto cfg = sens.GetConfig();

    // Vision cone
    if (showVision_) {
      const float half = (cfg.fovDeg * 0.5f) * (3.1415926f / 180.0f);
      const float a0 = rot - half;
      const float a1 = rot + half;
      drawFilledCone(pos, cfg.viewDistance, a0, a1);

      // Visible enemies and LOS
      const auto visibles = gw.Sense_VisibleEnemies();
      for (const auto& c : visibles) {
        Play::DrawCircle(c.pos, 4.0f, Play::cGreen);
        Play::DrawLine(pos, c.pos, Play::cGreen);
      }

      // Last known positions for non-visible enemies (fade by age)
      std::unordered_set<std::uint32_t> visSet; visSet.reserve(visibles.size());
      for (const auto& c : visibles) visSet.insert(c.id);

      for (const auto &otherId : sys.getAgents() | std::views::keys) {
        if (otherId == id) continue;
        if (visSet.contains(static_cast<std::uint32_t>(otherId))) continue;

        if (auto info = sens.Debug_LastKnownInfo((otherId))) {
          const float alpha01 = clamp01(1.0f - (info->ageSec / std::max(0.001f, cfg.memoryTTL)));
          const unsigned char a = u8(30.0f + alpha01 * 170.0f);
          const bool isHearing = (info->source == Sensing::MemorySource::Hearing);

          // Cyan for vision, Purple for hearing
          Play::Colour dotCol = isHearing ? Play::Colour{ 160, 64, 255, a }
                                           : Play::Colour{   0, 200, 255, a };

          Play::DrawCircle(info->pos, 4, dotCol);

          // Label with age seconds
          char label[48];
          std::snprintf(label, sizeof(label), "Last known (%.1fs)", info->ageSec);
          Play::DrawDebugText({ info->pos.x, info->pos.y + 14.0f }, label, 12, Play::cWhite);
        }
      }
    }

    // HUD tallies (per-agent) — always append regardless of showVision_/showAudio_
    // HUD tallies (per-agent)
    const auto counts = gw.Debug_GetEventCounts();
    hudLines_.push_back("T" + std::to_string(id) + "  Spotted:" + std::to_string(counts.spotted)
                        + "  Lost:" + std::to_string(counts.lost)
                        + "  Sounds:" + std::to_string(counts.sounds));
  }

  // Audio sublayer: draw all bus events globally (no per-tank dependency)
  if (showAudio_) {
    const auto* bus = sys.Debug_GetAudioBus();
    if (bus) {
      const auto events = bus->Debug_All();
      constexpr float ttl = 1.0f; // ~1s window
      for (const auto &ev : events) {
        const float alpha01 = std::clamp(1.0f - (ev.ageSec / ttl), 0.0f, 1.0f);
        const unsigned char a = u8(30.0f + alpha01 * 160.0f);
        Play::Colour col = (ev.kind == AI::SoundHeardEvent::Kind::Bullet)
                           ? Play::Colour{255, 64, 64, a}
                           : Play::Colour{  0,255,  0, a};
        Play::DrawCircle(ev.pos, static_cast<int>(ev.radius), col);
      }
      // Per-agent hearing links (dashed)
      for (const auto &agent : sys.getAgents() | std::views::values) {
        if (!agent.tank || !agent.gw) continue;
        const std::uint32_t selfId = static_cast<std::uint32_t>(agent.tank->GetID());
        const Play::Vector2D ear = agent.tank->GetPosition();
        const auto audible = bus->QueryInRadius(ear);
        for (const auto &ev : audible) {
          if (ev.sourceId == selfId) continue; // skip self
          const float dx = ev.pos.x - ear.x, dy = ev.pos.y - ear.y;
          const float len = std::sqrt(dx*dx + dy*dy);
          if (len <= 1e-3f) continue;
          const float ux = dx / len, uy = dy / len;
          float t = 0.0f; bool draw = true; constexpr int dash = 8, gap = 6;
          while (t < len) {
            const float seg = std::min<float>(dash, len - t);
            if (draw) {
              Play::Vector2D s{ ear.x + ux * t, ear.y + uy * t };
              Play::Vector2D e{ ear.x + ux * (t + seg), ear.y + uy * (t + seg) };
              Play::DrawLine(s, e, Play::Colour{255,255,255,140});
            }
            t += draw ? seg : gap; draw = !draw;
          }
        }
      }
    }
  }

  // Draw HUD block
  if (!hudLines_.empty()) {
    const int leftX = 120;
    int y = DISPLAY_HEIGHT - 110; // a bit below the legend
    for (const auto &s : hudLines_) {
      Play::DrawDebugText({ leftX, y }, s.c_str(), 12, Play::cWhite);
      y -= 14;
    }
  }
}

void SensingDebugLayer::handleInput(const AISubsystem& /*sys*/) {
  if (Play::KeyPressed(KEY_L)) showVision_ = !showVision_;
  if (Play::KeyPressed(KEY_H)) showAudio_  = !showAudio_;
}

} // namespace AI