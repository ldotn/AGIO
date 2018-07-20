#include <iostream>
#include <matplotlibcpp.h>
#include <algorithm>
#include "btBulletDynamicsCommon.h"

namespace plt = matplotlibcpp;
using namespace std;
#if 0
int main()
{
	plt::ion();

	btDefaultCollisionConfiguration* collision_configuration = new btDefaultCollisionConfiguration();
	btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collision_configuration);
	btBroadphaseInterface* overlapping_pair_cache = new btDbvtBroadphase();
	btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;

	btDiscreteDynamicsWorld* dynamics_world = new btDiscreteDynamicsWorld(dispatcher, overlapping_pair_cache, solver, collision_configuration);
	dynamics_world->setGravity(btVector3(0, -9.8, 0));

	// Note : Try to reuse collision shapes when possible
	btAlignedObjectArray<btCollisionShape*> collision_shapes;

	// Static sphere
	{
		btCollisionShape* ground_shape = new btSphereShape(btScalar(0.05));//new btBoxShape(btVector3(btScalar(50.), btScalar(0.1), btScalar(50.)));

		collision_shapes.push_back(ground_shape);

		btTransform ground_transform;
		ground_transform.setIdentity();
		ground_transform.setOrigin(btVector3(0, -2, 0));

		btScalar mass(0.);

		// Static so inertia (last parameter) is set to 0
		btDefaultMotionState* motion_state = new btDefaultMotionState(ground_transform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motion_state, ground_shape, { 0,0,0 });
		btRigidBody* body = new btRigidBody(rbInfo);
		body->setRestitution(0.6);

		//add the body to the dynamics world
		dynamics_world->addRigidBody(body);
	}

	// Dynamic sphere
	{
		btCollisionShape* col_shape = new btSphereShape(btScalar(0.05));
		collision_shapes.push_back(col_shape);

		// Create Dynamic Objects
		btTransform start_transform;
		start_transform.setIdentity();

		btScalar mass(1.f);

		// Object is dynamic so inertia needs to be computed
		btVector3 localInertia(0, 0, 0);
		col_shape->calculateLocalInertia(mass, localInertia);

		start_transform.setOrigin(btVector3(0, 0, 0));

		btDefaultMotionState* motion_state = new btDefaultMotionState(start_transform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motion_state, col_shape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);
		body->setRestitution(0.6);

		dynamics_world->addRigidBody(body);
	}

	// Need to separate them for the plot
	vector<float> positions_x(dynamics_world->getNumCollisionObjects());
	vector<float> positions_y(dynamics_world->getNumCollisionObjects());
	vector<float> positions_z(dynamics_world->getNumCollisionObjects());

	// Simulate and plot positions
	while (true)
	{
		dynamics_world->stepSimulation(1.f / 60.f, 10);

		for (int i = 0; i < dynamics_world->getNumCollisionObjects(); i++)
		{
			btCollisionObject* obj = dynamics_world->getCollisionObjectArray()[i];
			btRigidBody* body = btRigidBody::upcast(obj);

			// Assuming there's a valid motion state, one should check first
			btTransform trans;
			body->getMotionState()->getWorldTransform(trans);

			positions_x[i] = trans.getOrigin().getX();
			positions_y[i] = trans.getOrigin().getY();
			positions_z[i] = trans.getOrigin().getZ();
		}

		plt::clf();

		plt::plot(positions_x, positions_y, "ro");
		plt::ylim(-3, 1);
		plt::xlim(-2, 2);

		plt::pause(0.01);
	}

	// TODO : CLEAR EVERYTHING HERE!

	return 0;
}
#endif