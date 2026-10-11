#include <pch.h>
#include <engine/ecs/systems/physics_system.h>
#include <engine/ecs/Components.h>
#include <engine/ecs/Entity.h>

using namespace sc::ecs;
using namespace sc::ecs::physics_system;
namespace math = sc::math;
namespace physics = sc::physics;

constexpr float EPSILON = 0.001f;

namespace {
	struct Hit {
		math::SweepResult sweep;
		const physics::ColliderRef* other = nullptr;
	};

	// everything one UpdateKinematics call shares between the entities it moves
	struct StepContext {
		Registry& registry;
		sc::world::Level& level;
		physics::PhysicsScratch& scratch;
		float timeStep;
	};

	// the entity being moved this step
	struct Mover {
		entt::entity self;
		TransformComponent& trans;
		VelocityComponent& vel;
		const BoxColliderComponent& col;
	};

	// narrow phase: sweeps a moving entity against already resolved boxes.
	Hit FindClosestCollision(
		const StepContext& ctx,
		const Mover& mover,
		const sf::Vector2f& delta
	)
	{
		Hit closestHit{};

		for (const math::Hitbox& dynBox : mover.col.hitboxes) {
			if (dynBox.trigger) continue;

			const sf::FloatRect movingRect{ mover.trans.pos + dynBox.offset, dynBox.size };

			for (const physics::ColliderRef& other : ctx.scratch.candidates) {
				if (other.trigger) continue;

				const math::SweepResult sweep = math::SweptAABB(movingRect, delta, other.box);

				if (sweep.time < closestHit.sweep.time) {
					closestHit = Hit{ .sweep = sweep, .other = &other };
				}
			}
		}

		return closestHit;
	}

	sf::FloatRect SweptBounds(const sf::FloatRect& bounds, const sf::Vector2f& delta) {
		return sf::FloatRect{
			{
				bounds.position.x + std::min(0.0f, delta.x),
				bounds.position.y + std::min(0.0f, delta.y)
			},
			{
				bounds.size.x + std::abs(delta.x),
				bounds.size.y + std::abs(delta.y)
			}
		};
	}

	// resolves every entity overlapping area into world space boxes and appends them to
	// out, skipping self. the entity half of the broad phase; CollisionLayer::QueryArea
	// is the other half, and both append to the same buffer
	void EntityCollectCandidates(StepContext& ctx, const Mover& mover, const sf::FloatRect& area) {
		std::vector<EntityHandle>& gridHits = ctx.scratch.gridHits;
		ctx.level.GetSpatialGrid().Query(area, gridHits);

		for (EntityHandle other : gridHits) {
			if (other == mover.self) continue;

			const auto* otherTrans = ctx.registry.TryGetComponent<TransformComponent>(other);
			const auto* otherCol = ctx.registry.TryGetComponent<BoxColliderComponent>(other);
			if (!otherTrans || !otherCol) continue;

			for (const math::Hitbox& hitbox : otherCol->hitboxes) {
				ctx.scratch.candidates.push_back(physics::ColliderRef{
					.box = sf::FloatRect{ otherTrans->pos + hitbox.offset, hitbox.size },
					.type = physics::ColliderType::Entity,
					.trigger = hitbox.trigger,
					.data = { .entity = other }
				});
			}
		}
	}

	// both halves of the broad phase: static tiles from the collision layer, then entities from the spatial grid
	void CollectCandidates(StepContext& ctx, const Mover& mover, const sf::FloatRect& area) {
		ctx.scratch.candidates.clear(); ctx.scratch.gridHits.clear();
		ctx.level.GetCollisionLayer().QueryArea(area, ctx.scratch.candidates);
		EntityCollectCandidates(ctx, mover, area);
	}

	// pushes a mover that starts the step inside something solid back out
	// returns true if the mover was moved
	bool DepenetrationPass(StepContext& ctx, Mover& mover) {
		constexpr float SKIN = 0.01f; // margin so it doesn't touch the blocks
		constexpr int MAX_PUSHES = 4;

		bool moved = false;

		for (int i = 0; i < MAX_PUSHES; i++) {
			// deepest first
			sf::Vector2f deepestPush{};
			float deepestDepth = 0.f;

			for (const math::Hitbox& dynBox : mover.col.hitboxes) {
				if (dynBox.trigger) continue;

				const sf::FloatRect movingRect{ mover.trans.pos + dynBox.offset, dynBox.size };

				for (const physics::ColliderRef& other : ctx.scratch.candidates) {
					if (other.trigger) continue;

					const sf::Vector2f push = math::Penetration(movingRect, other.box);
					const float depth = std::abs(push.x) + std::abs(push.y);

					if (depth > deepestDepth) {
						deepestPush = push;
						deepestDepth = depth;
					}
				}
			}

			if (deepestDepth == 0.f) break;

			if (deepestPush.x != 0.f) deepestPush.x += (deepestPush.x > 0.f) ? SKIN : -SKIN;
			if (deepestPush.y != 0.f) deepestPush.y += (deepestPush.y > 0.f) ? SKIN : -SKIN;

			// prevPos is left alone so the render blends the correction over one step
			mover.trans.pos += deepestPush;
			moved = true;
		}

		return moved;
	}

	// runs both broad phases, calls the narrow phase, and handles the slide resolution
	void MoveAndResolve(StepContext& ctx, Mover& mover) {
		TransformComponent& trans = mover.trans;
		VelocityComponent& vel = mover.vel;
		const BoxColliderComponent& col = mover.col;

		sf::Vector2f delta = vel.velocity * ctx.timeStep;
		constexpr int MAX_SLIDES = 3;

		// broad phase
		CollectCandidates(ctx, mover, SweptBounds(EntityGetBounds(trans, col), delta));

		// depenetration pass recollect candidates if entities pushed
		if (DepenetrationPass(ctx, mover)) {
			CollectCandidates(ctx, mover, SweptBounds(EntityGetBounds(trans, col), delta));
		}

		// slide loop
		for (int i = 0; i < MAX_SLIDES; i++) {
			if (delta.x == 0.0f && delta.y == 0.0f) break;

			// narrow phase
			const Hit hit = FindClosestCollision(ctx, mover, delta);
			if (!hit.sweep.IsHit()) {
				trans.pos += delta;
				break;
			}

			// collect contacts
			ctx.scratch.contacts.push_back(physics::Contact{ mover.self, *hit.other, hit.sweep.normal, physics::ContactKind::Block });

			// sliding
			float safeTime = std::max(0.0f, hit.sweep.time - EPSILON);

			// move the entity up to the point of impact
			trans.pos += delta * safeTime;

			// calculate remaining time in this frame
			float remainingTime = 1.0f - hit.sweep.time;
			delta = delta * remainingTime;

			// remove velocity going into wall
			if (hit.sweep.normal.x != 0.0f) {
				delta.x = 0.0f;
				vel.velocity.x = 0.0f;
			}
			if (hit.sweep.normal.y != 0.0f) {
				delta.y = 0.0f;
				vel.velocity.y = 0.0f;
			}
		}

		// collect trigger contacts. only a solid box detects, and only someone else's trigger
		for (const math::Hitbox& dynBox : col.hitboxes) {
			if (dynBox.trigger) continue;

			const sf::FloatRect rect{ trans.pos + dynBox.offset, dynBox.size };

			for (const physics::ColliderRef& other : ctx.scratch.candidates) {
				if (!other.trigger) continue;

				if (rect.findIntersection(other.box)) {
					ctx.scratch.contacts.push_back(physics::Contact{ mover.self, other, {}, physics::ContactKind::Overlap });
				}
			}
		}
	}
}

namespace sc::ecs::physics_system {
	bool ColliderIsCollidable(const BoxColliderComponent& col) {
		return !col.hitboxes.empty();
	}

	sf::FloatRect EntityGetBounds(const TransformComponent& trans, const BoxColliderComponent& col) {
		assert(ColliderIsCollidable(col) && "GetEntityBounds requires at least one hitbox");

		// use first hitbox as initial values
		sf::Vector2f tl = col.hitboxes[0].offset;
		sf::Vector2f br = tl + col.hitboxes[0].size;

		for (const auto& hb : col.hitboxes) {
			// use most topleft pos for tl
			tl.x = std::min(tl.x, hb.offset.x);
			tl.y = std::min(tl.y, hb.offset.y);

			// use most bottom right pos for br
			br.x = std::max(br.x, hb.offset.x + hb.size.x);
			br.y = std::max(br.y, hb.offset.y + hb.size.y);
		}

		// convert tl + br to level space pos + size
		return sf::FloatRect{ trans.pos + tl, br - tl };
	}

	void EntityRegisterCollisions(math::SpatialGrid& grid, const Entity& entity) {
		// an entity without a collider is normal (markers, zones), so there is nothing to register
		if (!entity.HasComponent<TransformComponent>() || !entity.HasComponent<BoxColliderComponent>()) {
			return;
		}

		const auto& transform = entity.GetComponent<const TransformComponent>();
		const auto& collider = entity.GetComponent<const BoxColliderComponent>();

		// ensure collider has hitboxes
		if (!ColliderIsCollidable(collider)) {
			return;
		}

		grid.InsertEntity(entity.GetHandle(), EntityGetBounds(transform, collider));
	}

	void EntityRegisterCollisions(math::SpatialGrid& grid, EntityHandle handle, Registry& registry) {
		EntityRegisterCollisions(grid, Entity(handle, &registry));
	}

	void EntityUnregisterCollisions(math::SpatialGrid& grid, EntityHandle handle) {
		// non collidable entities are never inserted so only erase tracked ones
		if (grid.FindEntityCells(handle)) {
			grid.EraseEntity(handle);
		}
	}

	void UpdateKinematics(Registry& registry, world::Level& level, float timeStep, physics::PhysicsScratch& scratch) {
		StepContext ctx{ registry, level, scratch, timeStep };

		// grab all entities that can move. the collider is optional, so it isn't part of the view
		auto dynamicEntitiesView = registry.ViewLevel<
			TransformComponent,
			VelocityComponent
		>(level.GetUid());

		// clear previous contacts
		ctx.scratch.contacts.clear();

		for (auto [entity, trans, vel] : dynamicEntitiesView) {
			const sf::Vector2f oldPos = trans.pos;
			trans.prevPos = oldPos;

			// nothing to collide with and not in the spatial grid, so just move it
			const auto* col = registry.TryGetComponent<BoxColliderComponent>(entity);
			if (!col || !ColliderIsCollidable(*col)) {
				trans.pos += vel.velocity * timeStep;
				continue;
			}

			// move entities and handle collisions
			Mover mover{ entity, trans, vel, *col };
			MoveAndResolve(ctx, mover);

			// move entities in spatial grid
			if (trans.pos != oldPos) {
				level.GetSpatialGrid().MoveEntity(entity, EntityGetBounds(trans, *col));
			}
		}
	}
}
