//
// Copyright (c) 2008-2016 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "../Audio/Audio.h"
#include "../Audio/Sound.h"
#include "../Audio/SoundListener.h"
#include "../Audio/SoundSource3D.h"
#include "../IO/Log.h"
#include "../Core/Context.h"
#include "../Graphics/DebugRenderer.h"
#include "../Scene/Node.h"

#include "../IO/Log.h"

namespace Urho3D
{

static const float DEFAULT_NEARDISTANCE = 0.0f;
static const float DEFAULT_FARDISTANCE = 100.0f;
static const float DEFAULT_ROLLOFF = 2.0f;
static const float DEFAULT_ANGLE = 360.0f;
static const float MIN_ROLLOFF = 0.1f;
static const float DEFAULT_PANSPEED = 0.1f;
static const float DEFAULT_MAXPAN = 0.8f;
static const Color INNER_COLOR(1.0f, 0.5f, 1.0f);
static const Color OUTER_COLOR(1.0f, 0.0f, 1.0f);

extern const char* AUDIO_CATEGORY;

static Vector3 PointOnSphere(float radius, float theta, float phi)
{
    // Zero angles point toward positive Z axis
    phi += 90.0f;

    return Vector3(
        radius * Sin(theta) * Sin(phi),
        radius * Cos(phi),
        radius * Cos(theta) * Sin(phi)
    );
}

static void DrawDebugArc(const Vector3& worldPosition, const Quaternion& worldRotation, float angle, float distance, bool drawLines,
    const Color& color, DebugRenderer* debug, bool depthTest)
{
    if (angle <= 0.f)
        return;
    else if (angle >= 360.0f)
    {
        debug->AddSphere(Sphere(worldPosition, distance), color, depthTest);
        return;
    }

    unsigned uintColor = color.ToUInt();
    float halfAngle = 0.5f * angle;

    if (drawLines)
    {
        debug->AddLine(worldPosition, worldPosition + worldRotation * PointOnSphere(distance, halfAngle, halfAngle),
            uintColor);
        debug->AddLine(worldPosition, worldPosition + worldRotation * PointOnSphere(distance, -halfAngle, halfAngle),
            uintColor);
        debug->AddLine(worldPosition, worldPosition + worldRotation * PointOnSphere(distance, halfAngle, -halfAngle),
            uintColor);
        debug->AddLine(worldPosition, worldPosition + worldRotation * PointOnSphere(distance, -halfAngle, -halfAngle),
            uintColor);
    }

    const float step = 0.5f;

    for (float x = -1.0f; x < 1.0f; x += step)
    {
        debug->AddLine(worldPosition + worldRotation * PointOnSphere(distance, x * halfAngle, halfAngle),
            worldPosition + worldRotation * PointOnSphere(distance, (x + step) * halfAngle, halfAngle),
            uintColor);
        debug->AddLine(worldPosition + worldRotation * PointOnSphere(distance, x * halfAngle, -halfAngle),
            worldPosition + worldRotation * PointOnSphere(distance, (x + step) * halfAngle, -halfAngle),
            uintColor);
        debug->AddLine(worldPosition + worldRotation * PointOnSphere(distance, halfAngle, x * halfAngle),
            worldPosition + worldRotation * PointOnSphere(distance, halfAngle, (x + step) * halfAngle),
            uintColor);
        debug->AddLine(worldPosition + worldRotation * PointOnSphere(distance, -halfAngle, x * halfAngle),
            worldPosition + worldRotation * PointOnSphere(distance, -halfAngle, (x + step) * halfAngle),
            uintColor);
    }
}

SoundSource3D::SoundSource3D(Context* context) :
    SoundSource(context),
    nearDistance_(DEFAULT_NEARDISTANCE),
    farDistance_(DEFAULT_FARDISTANCE),
    rolloffFactor_(DEFAULT_ROLLOFF)

{
	
    // Start from zero volume until attenuation properly calculated
    attenuation_ = 0.0f;
}

void SoundSource3D::RegisterObject(Context* context)
{
    context->RegisterFactory<SoundSource3D>(AUDIO_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(SoundSource);
    // Remove Attenuation and Panning as attribute as they are constantly being updated
    URHO3D_REMOVE_ATTRIBUTE("Attenuation");
    URHO3D_REMOVE_ATTRIBUTE("Panning");
    URHO3D_ATTRIBUTE("Near Distance", float, nearDistance_, DEFAULT_NEARDISTANCE, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Far Distance", float, farDistance_, DEFAULT_FARDISTANCE, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Rolloff Factor", float, rolloffFactor_, DEFAULT_ROLLOFF, AM_DEFAULT);
}

void SoundSource3D::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
	if (!debug || !node_ || !IsEnabledEffective())
		return;

	const Matrix3x4& worldTransform = node_->GetWorldTransform();
	Vector3 worldPosition = worldTransform.Translation();
	Quaternion worldRotation = worldTransform.Rotation();

	// Draw cones for directional sounds, or spheres for non-directional

	debug->AddSphere(Sphere(worldPosition, nearDistance_), INNER_COLOR, depthTest);
	debug->AddSphere(Sphere(worldPosition, farDistance_), OUTER_COLOR, depthTest);
}

void SoundSource3D::Play(Sound* sound)
{
	
	MarkNetworkUpdate();
	if (sound != NULL)
	{
		
		SoLoud::Soloud* soloud = audio_->GetSoLoud();

		// Play the sound source (we could do this several times if we wanted)
		
		SoLoud::Wav *wav = sound->GetWav();
		
		wav->set3dAttenuator(&audio_->customAttenuator_);
		
		Vector3 p = node_->GetWorldPosition();
		
		handle_ = soloud->play3d(*wav, p.x_, p.y_, p.z_, 0.0f, 0.0f, 0.0f, 0.0f,false,0U);
		soloud->setVolume(handle_, gain_);
		//soloud->set3dSourcePosition(handle_ , p.x_ , p.y_ , p.z_);
		
		soloud->set3dSourceMinMaxDistance(handle_, nearDistance_, farDistance_);
		soloud->set3dSourceAttenuation(handle_, 1, rolloffFactor_);
		
		//soloud->set3dSourceDopplerFactor(handle_, 50.0f);
		soloud->setLooping(handle_, sound->IsLooped());
		
		URHO3D_LOGINFO("SoLoud Play3D: " + String(handle_) + " pos: " + String(p));
		//soloud->update3dVoices((unsigned int *)&handle_, 1);
		
	}
	else {
		URHO3D_LOGERROR("Unable to play sound, it is NULL");
	}
}


void SoundSource3D::Update(float timeStep)
{
	
	SoLoud::Soloud* soloud = audio_->GetSoLoud();
	Vector3 p = node_->GetWorldPosition();
	soloud->set3dSourcePosition(handle_, p.x_, p.y_, p.z_);
	
	SoundSource::Update(timeStep);

}

void SoundSource3D::SetDistanceAttenuation(float nearDistance, float farDistance, float rolloffFactor)
{
    nearDistance_ = Max(nearDistance, 0.0f);
    farDistance_ = Max(farDistance, 0.0f);
    rolloffFactor_ = Max(rolloffFactor, MIN_ROLLOFF);
    MarkNetworkUpdate();
}

void SoundSource3D::SetFarDistance(float distance)
{
    farDistance_ = Max(distance, 0.0f);
    MarkNetworkUpdate();
}

void SoundSource3D::SetNearDistance(float distance)
{
    nearDistance_ = Max(distance, 0.0f);
    MarkNetworkUpdate();
}

void SoundSource3D::SetRolloffFactor(float factor)
{
    rolloffFactor_ = Max(factor, MIN_ROLLOFF);
    MarkNetworkUpdate();
}

}
