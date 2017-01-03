#include <entityx/entityx.h>
#include "systems_upd.h"
#include "../components.h"
#include "../cvar_game.h"

void PlayerSystem::update(ex::EntityManager &es, ex::EventManager &events, ex::TimeDelta dt) {
	for (auto ent : es.entities_with_components<PlayerInput, Body, Movable>()) {
		auto input = ent.component<PlayerInput>();
		auto body = ent.component<Body>();
		auto speed = ent.component<Movable>();

		// FIXME reset didjump/candoublejump if grounded

		speed->dy += p_gravity->value * dt;

		// set wallsliding to false
		// check if can walljump

		// if falling down and can walljump (since you're touching the wall) go wallslide

		// set speed if wallsliding

		if (input->right || input->left) {
			speed->dx += (input->right ? p_accel->value : input->left ? -p_accel->value : 0) * dt;
		} else if (speed->dx != 0) {
			auto friction = p_groundFriction->value * dt;
			if (friction > fabs(speed->dx)) {
				speed->dx = 0;
			} else {
				speed->dx += friction * (speed->dx > 0 ? -1 : 1);
			}
		}

		if (input->up || input->down) {
			speed->dy = input->down ? p_accel->value : input->up ? -p_accel->value : 0 * dt;
		}

		speed->dx = clamp(speed->dx, -p_maxSpeed->value, p_maxSpeed->value);
		auto uncappedY = speed->dy;
		speed->dy = clamp(speed->dy, -p_terminalVelocity->value, p_terminalVelocity->value);

		// do the move and collision checks
		ex::Entity hitEnt;

		Sweep move = Move(&es, ent, speed->dx * dt, speed->dy * dt, hitEnt);

		body->pos.x = move.pos.x;
		body->pos.y = move.pos.y;

		if (move.hit.valid) {
			speed->dx = move.hit.normal.x ? 0 : speed->dx;
			speed->dy = move.hit.normal.y ? 0 : speed->dy;
			
			auto remain = 1.0 - move.time;
			auto remx = move.hit.normal.x != 0 ? 0 : speed->dx * dt * remain;
			auto remy = move.hit.normal.y != 0 ? 0 : speed->dy * dt * remain;
			Sweep moveremain = Move(&es, ent, remx, remy, hitEnt);

			body->pos.x = moveremain.pos.x;
			body->pos.y = moveremain.pos.y;
		}

		if (fabs(speed->dx) < 0.2) {
			speed->dx = 0;
		}

		if (fabs(speed->dy) < 0.2) {
			speed->dy = 0;
		}
	}
}