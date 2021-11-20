///--------------------------------------------------------------------------------------------------------------------------------------------------
/// Author: Andrew Chase
/// Date Created: October 15th, 2021
/// Description: 
///--------------------------------------------------------------------------------------------------------------------------------------------------

///--------------------------------------------------------------------------------------------------------------------------------------------------
/// INCLUDES
///--------------------------------------------------------------------------------------------------------------------------------------------------
#include "Game/Entity/Player.h"
#include "Engine/Collision/Collider.h"
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
	transform.position = Vector3(0.f, 0.f, -2.f);
	m_camera->SetPosition(transform.position + s_cameraOffset);
	m_camera->SetRotationEulerAnglesDegrees(Vector3::ZERO);

	collider = new CapsuleCollider(this, Capsule3(Vector3(0.f, 0.5f, 0.f), Vector3(0.f, 1.5f, 0.f), 0.5f));

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
	if (g_inputSystem->WasKeyJustPressed('P'))
	{
		m_cameraEjected = !m_cameraEjected;

		if (!m_cameraEjected)
		{
			m_camera->SetPosition(transform.position + s_cameraOffset);
			m_camera->SetRotationEulerAnglesDegrees(transform.rotation.GetAsEulerAnglesDegrees());
		}
	}

	// Translation input
	Vector3 moveDir = Vector3::ZERO;
	if (g_inputSystem->IsKeyPressed('W')) { moveDir.z += 1.f; }		// Forward
	if (g_inputSystem->IsKeyPressed('S')) { moveDir.z -= 1.f; }		// Left
	if (g_inputSystem->IsKeyPressed('A')) { moveDir.x -= 1.f; }		// Back
	if (g_inputSystem->IsKeyPressed('D')) { moveDir.x += 1.f; }		// Right

	// Rotation input
	Mouse& mouse = InputSystem::GetMouse();
	IntVector2 mouseDelta = mouse.GetMouseDelta();
	Vector2 rot = Vector2((float)mouseDelta.y, (float)mouseDelta.x); // Flip X and Y

	const float degreesPerSecond = 30.f;
	Vector3 deltaDegrees = Vector3(rot.x, rot.y, 0.f) * degreesPerSecond * deltaSeconds;

	if (m_cameraEjected)
	{
		if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_SPACEBAR)) { moveDir.y += 1.f; }		// Up
		if (g_inputSystem->IsKeyPressed('X')) { moveDir.y -= 1.f; }		// Down
		moveDir.SafeNormalize(moveDir);

		float scale = 10.f;
		if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_SHIFT))
		{
			scale = 20.f;
		}
		else if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_CONTROL))
		{
			scale = 5.f;
		}

		m_camera->Translate(moveDir * scale * deltaSeconds);
		Vector3 cameraDegrees = m_camera->GetRotationAsEulerAnglesDegrees() + Vector3(deltaDegrees.x, deltaDegrees.y, 0.f);

		m_camera->SetRotationEulerAnglesDegrees(cameraDegrees);

		rigidBody->SetAcceleration(Vector3::ZERO);
	}
	else
	{
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

		transform.SetRotation(transform.GetWorldRotation().GetAsEulerAnglesDegrees() + Vector3(0.f, deltaDegrees.y, 0.f));

		Vector3 cameraDegrees = m_camera->GetRotationAsEulerAnglesDegrees() + Vector3(deltaDegrees.x, deltaDegrees.y, 0.f);
		cameraDegrees.x = Clamp(cameraDegrees.x, -70.f, 70.f);

		m_camera->SetRotationEulerAnglesDegrees(cameraDegrees);

		if (g_inputSystem->WasKeyJustPressed(InputSystem::KEYBOARD_SPACEBAR))
		{
			rigidBody->AddWorldVelocity(Vector3(0.f, 5.f, 0.f));
		}
	}
}


//-------------------------------------------------------------------------------------------------
void Player::PreUpdate(float deltaSeconds)
{
	Entity::PreUpdate(deltaSeconds);
}


//-------------------------------------------------------------------------------------------------
void Player::PostUpdate(float deltaSeconds)
{
	Entity::PostUpdate(deltaSeconds);

	Vector3 pos;
	float speed = 0.f;

	if (m_cameraEjected)
	{
		pos = m_camera->GetPosition();
		speed = 10.f;
		if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_SHIFT))
		{
			speed = 20.f;
		}
		else if (g_inputSystem->IsKeyPressed(InputSystem::KEYBOARD_CONTROL))
		{
			speed = 5.f;
		}

		ConsolePrintf("Camera ejected - Press 'P' to un-eject");
	}
	else
	{
		m_camera->SetPosition(transform.position + s_cameraOffset);

		pos = transform.position;
		speed = rigidBody->GetVelocityWs().GetLength();

		ConsolePrintf("Camera fixed to player - Press 'P' to eject");
	}

	if (AreMostlyEqual(pos.x, 0.f)) { pos.x = 0.f; }
	if (AreMostlyEqual(pos.y, 0.f)) { pos.y = 0.f; }
	if (AreMostlyEqual(pos.z, 0.f)) { pos.z = 0.f; }
	if (AreMostlyEqual(speed, 0.f)) { speed = 0.f; }

	ConsolePrintf("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
	ConsolePrintf("Speed: %.2f m/s", speed);
}


//-------------------------------------------------------------------------------------------------
void Player::Render() const
{
}
