///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: October 15th, 2021
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Game/Entity/Player.h"
#include "Engine/Core/DevConsole.h"
#include "Engine/Core/EngineCommon.h"
#include "Engine/IO/InputSystem.h"
#include "Engine/Math/MathUtils.h"
#include "Engine/Physics/Rigidbody/Rigidbody.h"
#include "Engine/Render/Camera.h"

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// DEFINES
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// ENUMS, TYPEDEFS, STRUCTS, FORWARD DECLARATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// GLOBALS AND STATICS
///--------------------------------------------------------------------------------------------------------------------------------------------------
Vector3 Player::s_cameraOffset = Vector3(0.f, 1.5f, 0.f);
float Player::s_maxMoveSpeed = 5.0f;

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// C FUNCTIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// CLASS IMPLEMENTATIONS
///--------------------------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
Player::Player(Camera* camera)
	: m_camera(camera)
{
	m_camera->transform.SetParentTransform(&transform);
	m_camera->SetPosition(s_cameraOffset);
	m_camera->SetRotationEulerAnglesDegrees(Vector3::ZERO);

	collider = new CapsuleCollider(this, Capsule3D(Vector3(0.f, 0.5f, 0.f), Vector3(0.f, 1.5f, 0.f), 0.5f));

	rigidBody = new RigidBody(&transform);
	rigidBody->SetInverseMass(1.f);
	rigidBody->SetInertiaTensor_Capsule(1.0f, 1.0f);
	rigidBody->SetAffectedByGravity(true);
	rigidBody->SetRotationLocked(true);
	rigidBody->SetMaxLateralSpeed(s_maxMoveSpeed);
	rigidBody->SetCanSleep(false);
}


//-------------------------------------------------------------------------------------------------
Player::~Player()
{
	SAFE_DELETE(collider);
}


//-------------------------------------------------------------------------------------------------
void Player::ProcessInput(float deltaSeconds)
{
	static Vector3 vel = Vector3::ZERO;

	// Translate
	Vector3 moveDir = Vector3::ZERO;
	if (g_inputSystem->IsKeyPressed('W')) { moveDir.z += 1.f; }		// Forward
	if (g_inputSystem->IsKeyPressed('S')) { moveDir.z -= 1.f; }		// Left
	if (g_inputSystem->IsKeyPressed('A')) { moveDir.x -= 1.f; }		// Back
	if (g_inputSystem->IsKeyPressed('D')) { moveDir.x += 1.f; }		// Right
	moveDir.SafeNormalize(moveDir);

	rigidBody->SetAcceleration(transform.TransformDirection(moveDir * 50.f));

	if (g_inputSystem->WasKeyJustPressed(InputSystem::KEYBOARD_SHIFT))
	{
		rigidBody->SetMaxLateralSpeed(2.f * s_maxMoveSpeed);
	}
	else if (g_inputSystem->WasKeyJustReleased(InputSystem::KEYBOARD_SHIFT))
	{
		rigidBody->SetMaxLateralSpeed(s_maxMoveSpeed);
	}

	// Rotate
	Mouse& mouse = InputSystem::GetMouse();
	IntVector2 mouseDelta = mouse.GetMouseDelta();
	Vector2 rot = Vector2((float)mouseDelta.y, (float)mouseDelta.x); // Flip X and Y

	const float degreesPerSecond = 30.f;
	Vector3 deltaDegrees = Vector3(rot.x, rot.y, 0.f) * degreesPerSecond * deltaSeconds;
	transform.SetRotation(transform.GetWorldRotation().GetAsEulerAnglesDegrees() + Vector3(0.f, deltaDegrees.y, 0.f));

	Vector3 cameraDegrees = m_camera->GetRotationAsEulerAnglesDegrees() + Vector3(deltaDegrees.x, 0.f, 0.f);
	cameraDegrees.x = Clamp(cameraDegrees.x, -70.f, 70.f);

	m_camera->SetRotationEulerAnglesDegrees(cameraDegrees);

	if (g_inputSystem->WasKeyJustPressed(InputSystem::KEYBOARD_SPACEBAR))
	{
		rigidBody->AddWorldVelocity(Vector3(0.f, 5.f, 0.f));
		//rigidBody->AddLocalForce(Vector3::Y_AXIS * 1000.f);
	}
}


//-------------------------------------------------------------------------------------------------
void Player::Update(float deltaSeconds)
{
	Entity::Update(deltaSeconds);
}


//-------------------------------------------------------------------------------------------------
void Player::Render() const
{
	Vector3 lateralVelocity = rigidBody->GetVelocityWs();
	lateralVelocity.y = 0.f;
	ConsolePrintf("Speed: %.2f", lateralVelocity.GetLength());
}
