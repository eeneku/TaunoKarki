#include "gameobjectmanager.h"
#include "gameobject.h"

#include <algorithm>
#include <random>

#include "transform.h"
#include "circlecollider.h"
#include "meshrenderer.h"
#include "rigidbody.h"
#include "playercontroller.h"
#include "boxcollider.h"
#include "staticbody.h"
#include "health.h"
#include "aicontroller.h"
#include "damage.h"
#include "healthbar.h"
#include "ammobar.h"
#include "collectible.h"

#include "pistol.h"
#include "machinegun.h"
#include "shotgun.h"

#include <iostream>

#define randomInt std::uniform_int_distribution<int>

GameObjectManager::GameObjectManager(AssetManager& assetManager, b2World& world, Camera& camera) : assetManager(assetManager), world(world), camera(camera)
{
}

GameObjectManager::~GameObjectManager()
{
	for (auto gameObject : gameObjects)
	{
		delete gameObject;
	}

	gameObjects.clear();
	newObjects.clear();
}

void GameObjectManager::addNewObject(std::function< void(void)> gameObject)
{
	newObjects.push_back(gameObject);
}

void GameObjectManager::update()
{
	for (auto gameObject : gameObjects)
	{
		gameObject->update();
		if (!gameObject->isAlive()) deadObjects.push_back(gameObject);
	}

	createObjects();
	deleteObjects();
}

void GameObjectManager::createObjects()
{
	for (auto newObject : newObjects)
	{
		newObject();
	}

	newObjects.clear();
}

void GameObjectManager::deleteObjects()
{
	gameObjects.erase(std::remove_if(gameObjects.begin(), gameObjects.end(), [](GameObject *obj){ return !obj->isAlive(); }), gameObjects.end());

	for (auto gameObject : deadObjects)
	{
		delete gameObject;
	}

	deadObjects.clear();
}

void GameObjectManager::draw()
{
	for (auto gameObject : gameObjects)
		gameObject->draw();
}

GameObject* GameObjectManager::createBullet(glm::vec3 position, glm::vec2 direction, unsigned int owner, float damage, float speed)
{
	GameObject* gameObject = createObject();

	gameObject->setType(owner);

	gameObject->addComponent(new Transform(gameObject, position.x + direction.x, position.y + direction.y, 0));
	if (owner == GAMEOBJECT_TYPES::PLAYER_BULLET)  gameObject->addComponent(new CircleCollider(gameObject, 0.05f, COL_PLAYER_BULLET, ( COL_WALL | COL_ENEMY )));
	else  gameObject->addComponent(new CircleCollider(gameObject, 0.05f, COL_ENEMY_BULLET, (COL_WALL | COL_PLAYER)));
	gameObject->addComponent(new RigidBody(gameObject, world));
	gameObject->addComponent(new Damage(gameObject, damage));
	gameObject->addDrawableComponent(new MeshRenderer(gameObject));

	gameObject->getComponent<Transform>()->setScale(glm::vec3(0.05f));
	gameObject->getDrawableComponent<MeshRenderer>()->setMesh(assetManager.sphereMesh);
	gameObject->getDrawableComponent<MeshRenderer>()->setTexture(assetManager.sphereTexture);
	gameObject->getDrawableComponent<MeshRenderer>()->setViewMatrix(camera.getViewMatrix());
	gameObject->getDrawableComponent<MeshRenderer>()->setProjectionMatrix(camera.getPerspectiveMatrix());
	gameObject->getDrawableComponent<MeshRenderer>()->setProgram(assetManager.shaderProgram);

	b2Body* body = gameObject->getComponent<RigidBody>()->getBody();
	body->SetBullet(true);
	b2Vec2 forceDir(direction.x, direction.y);
	forceDir *= speed;
	body->ApplyLinearImpulse(forceDir, body->GetWorldCenter(), true);

	return gameObject;
}

GameObject* GameObjectManager::createPlayer(glm::vec3 position, Weapon* weapon)
{
 	GameObject* gameObject = createObject();

	gameObject->setType(GAMEOBJECT_TYPES::PLAYER);

	gameObject->addComponent(new Transform(gameObject, position.x, position.y, position.z));
	gameObject->addComponent(new CircleCollider(gameObject, 0.5f, COL_PLAYER, (COL_WALL | COL_ENEMY | COL_ENEMY_BULLET)));
	gameObject->addComponent(new RigidBody(gameObject, world));
	gameObject->addComponent(new PlayerController(gameObject));
	gameObject->addComponent(new Health(gameObject, 100));
	gameObject->addDrawableComponent(new MeshRenderer(gameObject));

	gameObject->getComponent<Transform>()->setScale(glm::vec3(0.5f, 0.5f, 0.75f));
	gameObject->getDrawableComponent<MeshRenderer>()->setMesh(assetManager.cubeMesh);
	gameObject->getDrawableComponent<MeshRenderer>()->setProgram(assetManager.shaderProgram);
	gameObject->getDrawableComponent<MeshRenderer>()->setViewMatrix(camera.getViewMatrix());
	gameObject->getDrawableComponent<MeshRenderer>()->setProjectionMatrix(camera.getPerspectiveMatrix());
	gameObject->getDrawableComponent<MeshRenderer>()->setTexture(assetManager.playerTexture);
	gameObject->getComponent<RigidBody>()->getBody()->SetFixedRotation(true);

	if (weapon)
		gameObject->getComponent<PlayerController>()->giveWeapon(weapon);
	else
		gameObject->getComponent<PlayerController>()->giveWeapon(new Pistol(*this));

	return gameObject;
}

GameObject* GameObjectManager::createEnemy(glm::vec3 position)
{
	GameObject* gameObject = createObject();

	gameObject->setType(GAMEOBJECT_TYPES::ENEMY);

	gameObject->addComponent(new Transform(gameObject, position.x, position.y, position.z));
	gameObject->addComponent(new CircleCollider(gameObject, 0.5f, COL_ENEMY, (COL_WALL | COL_PLAYER | COL_PLAYER_BULLET)));
	gameObject->addComponent(new RigidBody(gameObject, world));
	gameObject->addComponent(new Health(gameObject, 100));
	gameObject->addComponent(new AIController(gameObject));
	gameObject->addComponent(new Damage(gameObject, 50.0f));
	gameObject->addDrawableComponent(new MeshRenderer(gameObject));

	gameObject->getComponent<Transform>()->setScale(glm::vec3(0.5f, 0.5f, 0.75f));
	gameObject->getDrawableComponent<MeshRenderer>()->setMesh(assetManager.cubeMesh);
	gameObject->getDrawableComponent<MeshRenderer>()->setProgram(assetManager.shaderProgram);
	gameObject->getDrawableComponent<MeshRenderer>()->setViewMatrix(camera.getViewMatrix());
	gameObject->getDrawableComponent<MeshRenderer>()->setProjectionMatrix(camera.getPerspectiveMatrix());
	gameObject->getDrawableComponent<MeshRenderer>()->setTexture(assetManager.enemyTexture);
	gameObject->getComponent<RigidBody>()->getBody()->SetFixedRotation(true);
	gameObject->getComponent<AIController>()->giveWeapon(new Pistol(*this));

	return gameObject;
}

GameObject* GameObjectManager::createPlayerHealthBar(glm::vec3 position, glm::vec2 size)
{
	GameObject* gameObject = createObject();

	gameObject->setType(GAMEOBJECT_TYPES::GUI);

	gameObject->addComponent(new Transform(gameObject, 0.0f, 0.0f, 0.0f));
	gameObject->addComponent(new HealthBar(gameObject));
	gameObject->addDrawableComponent(new MeshRenderer(gameObject));

	Mesh* mesh = new Mesh(*assetManager.floorMesh);
	assetManager.addMesh(mesh);

	for (Vertex& vertex : mesh->getVertices())
	{
		if(vertex.position.x > 0.0f) vertex.position.x *= size.x;
		else if (vertex.position.x < 0.0f) vertex.position.x = 0.0f;
		vertex.position.y *= size.y;
	}

	gameObject->getComponent<HealthBar>()->setOffsetPosition(position);

	MeshRenderer* meshRenderer = gameObject->getDrawableComponent<MeshRenderer>();
	meshRenderer->setMesh(mesh);
	meshRenderer->setProgram(assetManager.shaderProgram);
	meshRenderer->setTexture(assetManager.sphereTexture);
	meshRenderer->setViewMatrix(camera.getViewMatrix());
	meshRenderer->setProjectionMatrix(camera.getPerspectiveMatrix());

	return gameObject;
}

GameObject* GameObjectManager::createPlayerAmmoBar(glm::vec3 position, glm::vec2 size)
{
	GameObject* gameObject = createObject();

	gameObject->setType(GAMEOBJECT_TYPES::GUI);

	gameObject->addComponent(new Transform(gameObject, 0.0f, 0.0f, 0.0f));
	gameObject->addComponent(new AmmoBar(gameObject));
	gameObject->addDrawableComponent(new MeshRenderer(gameObject));

	Mesh* mesh = new Mesh(*assetManager.floorMesh);
	assetManager.addMesh(mesh);

	for (Vertex& vertex : mesh->getVertices())
	{
		if (vertex.position.x < 0.0f) vertex.position.x *= size.x;
		else if (vertex.position.x > 0.0f) vertex.position.x = 0.0f;
		vertex.position.y *= size.y;
	}

	gameObject->getComponent<AmmoBar>()->setOffsetPosition(position);

	MeshRenderer* meshRenderer = gameObject->getDrawableComponent<MeshRenderer>();
	meshRenderer->setMesh(mesh);
	meshRenderer->setProgram(assetManager.shaderProgram);
	meshRenderer->setTexture(assetManager.sphereTexture);
	meshRenderer->setViewMatrix(camera.getViewMatrix());
	meshRenderer->setProjectionMatrix(camera.getPerspectiveMatrix());

	return gameObject;
}

GameObject* GameObjectManager::createRandomItem(glm::vec3 position)
{
	GameObject* gameObject = createObject();

	gameObject->setType(GAMEOBJECT_TYPES::ITEM);

	gameObject->addComponent(new Transform(gameObject, position.x, position.y, position.z));
	gameObject->addComponent(new CircleCollider(gameObject, 0.25f, COL_WALL, (COL_PLAYER)));
	gameObject->addComponent(new RigidBody(gameObject, world));
	gameObject->addComponent(new Collectible(gameObject));
	gameObject->addDrawableComponent(new MeshRenderer(gameObject));

	std::random_device randomDevice;
	std::default_random_engine randomGenerator(randomDevice());

	switch (randomInt(0, 3)(randomGenerator))
	{
	case 0: gameObject->getComponent<Collectible>()->setType(COLLECTIBLES::PISTOL); gameObject->getDrawableComponent<MeshRenderer>()->setMesh(assetManager.pistolMesh); break;
	case 1: gameObject->getComponent<Collectible>()->setType(COLLECTIBLES::MACHINEGUN); gameObject->getDrawableComponent<MeshRenderer>()->setMesh(assetManager.machinegunMesh); break;
	case 2: gameObject->getComponent<Collectible>()->setType(COLLECTIBLES::SHOTGUN); gameObject->getDrawableComponent<MeshRenderer>()->setMesh(assetManager.shotgunMesh); break;
	case 3: gameObject->getComponent<Collectible>()->setType(COLLECTIBLES::HEALTHPACK); gameObject->getDrawableComponent<MeshRenderer>()->setMesh(assetManager.healthpackMesh); break;
	default: break;
	}

	gameObject->getComponent<Transform>()->setScale(glm::vec3(0.35f, 0.35f, 0.35f));
	gameObject->getDrawableComponent<MeshRenderer>()->setProgram(assetManager.shaderProgram);
	gameObject->getDrawableComponent<MeshRenderer>()->setViewMatrix(camera.getViewMatrix());
	gameObject->getDrawableComponent<MeshRenderer>()->setProjectionMatrix(camera.getPerspectiveMatrix());
	gameObject->getDrawableComponent<MeshRenderer>()->setTexture(assetManager.itemsTexture);



	gameObject->getComponent<RigidBody>()->getBody()->SetFixedRotation(true);

	return gameObject;
}

GameObject* GameObjectManager::getFirstObjectOfType(size_t type) const
{
	for (GameObject* gameObject : gameObjects)
	{
		if (gameObject->getType() == type) return gameObject;
	}

	return nullptr;
}

size_t GameObjectManager::getNumberOfObjectsOfType(size_t type) const
{
	size_t counter = 0;

	for (GameObject* gameObject : gameObjects)
	{
		if (gameObject->getType() == type) counter++;
	}

	return counter;
}

void GameObjectManager::interpolate(float alpha)
{
	for (GameObject* gameObject : gameObjects)
	{
		Transform* transform = gameObject->getComponent<Transform>();
		RigidBody* rigidBody = gameObject->getComponent<RigidBody>();

		// We are only interested in moving objects
		if (transform && rigidBody)
		{
			glm::vec3 position = transform->getPosition();
			b2Vec2 bodyPosition = rigidBody->getBody()->GetPosition();

			position.x = bodyPosition.x * alpha + position.x * (1.0f - alpha);
			position.y = bodyPosition.y * alpha + position.y * (1.0f - alpha);

			transform->setPosition(position);
		}
	}
}

GameObject* GameObjectManager::createObject()
{
	GameObject* gameObject = new GameObject(*this);
	gameObjects.push_back(gameObject);

	return gameObject;
}