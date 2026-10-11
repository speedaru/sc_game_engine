#include <pch.h>
#include <systems/collision_system.h>

#include <engine/utils/logging.h>
#include <engine/ecs/Entity.h>

#include <components/GameComponents.h>

using namespace sc;
namespace phys = physics;

namespace game::collision_system {
	namespace {
		struct HandlerContext {
			ecs::Registry& registry;
			const phys::Contact& contact;
			ContactState& state;
		};

		bool PairEqualsContact(const PairCooldown& pair, const phys::Contact& contact) {
			return pair.self == contact.self // self equals
				&& contact.other.type == phys::ColliderType::Entity // other is entity
				&& pair.other == contact.other.GetEntity(); // other entity equals
		}

		void HandleUnknownContact(HandlerContext& ctx) {
			LOG_W("unknown contact type on entity: %u", static_cast<uint32_t>(ctx.contact.self));
		}

		void HandleOverlapContact(HandlerContext& ctx) {
			auto& reg = ctx.registry;

			if (ctx.contact.other.type == phys::ColliderType::Entity) {
				ecs::EntityHandle self = ctx.contact.self;
				ecs::EntityHandle other = ctx.contact.other.GetEntity();

				auto rateLimit = [&ctx, &self, &other](float cooldown) {
					// dedup multiple hitboxes per entity
					ctx.state.rateLimits.push_back(PairCooldown{ .self = self, .other = other, .timeLeft = cooldown });
				};

				// pickup component
				if (reg.HasComponent<components::PlayerTag>(self) && reg.HasComponent<components::PickupTag>(other)) {
					reg.QueueDestroy(other);
					rateLimit(0.f);
				}

				// teleport component. only the player, since applying it switches the viewed level
				if (reg.HasComponent<components::PlayerTag>(self)) {
					if (const auto* teleport = reg.TryGetComponent<components::TeleportComponent>(other)) {
						ctx.state.requests.push_back(TeleportRequest{ teleport->levelUid, teleport->pos });
						rateLimit(0.f);
					}
				}
			}
		}
	}

	void DispatchContacts(
		sc::ecs::Registry& registry,
		const std::vector<sc::physics::Contact>& contacts,
		ContactState& state,
		float timeStep
	) {
		std::vector<PairCooldown>& rateLimits = state.rateLimits;

		// decrease rateLimits by timeStep and remove finished limits
		for (auto& pair : rateLimits) { pair.timeLeft -= timeStep; }
		auto newEnd = std::remove_if(rateLimits.begin(), rateLimits.end(), [](auto& pair) { return pair.timeLeft <= 0.f; });
		rateLimits.erase(newEnd, rateLimits.end());

		for (const auto& contact : contacts) {
			auto comparePair = [&contact](const auto& pair) { return PairEqualsContact(pair, contact); };
			if (std::find_if(rateLimits.begin(), rateLimits.end(), comparePair) != rateLimits.end()) {
				// skip contacts that still have a cooldown
				continue;
			}

			HandlerContext ctx{ registry, contact, state };

			switch (contact.kind) {
				case phys::ContactKind::Block:
					// nothing reacts to being blocked yet
					break;
				case phys::ContactKind::Overlap:
					HandleOverlapContact(ctx);
					break;
				case phys::ContactKind::Unknown:
					HandleUnknownContact(ctx);
					break;
				default:
					LOG_W("no handler found for contact type: %u", static_cast<uint32_t>(contact.kind));
					break;
			}
		}
	}
}
