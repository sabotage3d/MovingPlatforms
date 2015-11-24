//
//  Platform.h
//  PlatformTest
//
//  Created by Alexander on 23/11/2015.
//
//

#ifndef __PlatformTest__Platform__
#define __PlatformTest__Platform__

#include <Urho3D/Urho3D.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Math/Vector3.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Math/Vector2.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/UI/Window.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Audio/SoundSource3D.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Scene/LogicComponent.h>
#include <Urho3D/Scene/LogicComponent.h>

// All Urho3D classes reside in namespace Urho3D
using namespace Urho3D;

/// Custom logic component for rotating a scene node.
class Platform : public LogicComponent
{
    OBJECT(Platform);
    
public:
    /// Construct.
    Platform(Context* context);
    
    virtual void Start();

    /// Handle scene update. Called by LogicComponent base class.
    virtual void Update(float timeStep);
    virtual void HandleNodeCollision(StringHash eventType, VariantMap& eventData);
    void SetId(int id);

    
    
private:
    Vector3 direction_;
    float elapsedTime_;
    int id_;

   
};

#endif /* defined(__PlatformTest__Platform__) */
