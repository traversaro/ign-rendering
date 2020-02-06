/*
 * Copyright (C) 2018 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// Note this include is placed in the src file because
// otherwise ogre produces compile errors
#include <Hlms/Pbs/OgreHlmsPbsDatablock.h>

#include <ignition/common/Console.hh>

#include "ignition/rendering/ogre2/Ogre2Conversions.hh"
#include "ignition/rendering/ogre2/Ogre2Mesh.hh"
#include "ignition/rendering/ogre2/Ogre2Includes.hh"
#include "ignition/rendering/ogre2/Ogre2Material.hh"
#include "ignition/rendering/ogre2/Ogre2Storage.hh"


/// brief Private implementation of the Ogre2Mesh class
class ignition::rendering::Ogre2MeshPrivate
{
};

using namespace ignition;
using namespace rendering;

//////////////////////////////////////////////////
Ogre2Mesh::Ogre2Mesh()
  : dataPtr(new Ogre2MeshPrivate)
{
}

//////////////////////////////////////////////////
Ogre2Mesh::~Ogre2Mesh()
{
}

//////////////////////////////////////////////////
void Ogre2Mesh::Destroy()
{
  if (!this->ogreItem)
    return;

  // We need to override BaseMesh::Destroy for ogre2 implementation to control
  // the order in which ogre items and materials are destroyed.
  // Items must be destroyed before materials otherwise ogre throws an exception
  // when unlinking an renderable from a hlms datablock

  // Remove this object from parent
  BaseGeometry::Destroy();

  // destroy mesh (ogre item)
  auto ogreScene = std::dynamic_pointer_cast<Ogre2Scene>(this->Scene());
  ogreScene->OgreSceneManager()->destroyItem(this->ogreItem);
  this->ogreItem = nullptr;

  // destroy submeshes (ogre subitems)
  this->SubMeshes()->DestroyAll();

  // destroy material (ogre hlms datablock) - this needs to be done last!
  if (this->material && this->ownsMaterial)
    this->Scene()->DestroyMaterial(this->material);
  this->material.reset();
}

//////////////////////////////////////////////////
bool Ogre2Mesh::HasSkeleton() const
{
  return this->ogreItem->hasSkeleton();
}

//////////////////////////////////////////////////
std::map<std::string, math::Matrix4d> Ogre2Mesh::SkeletonLocalTransforms() const
{
  std::map<std::string, ignition::math::Matrix4d> mapTfs;
  if (this->ogreItem->hasSkeleton())
  {
    auto skel = this->ogreItem->getSkeletonInstance();
    for (unsigned int i = 0; i < skel->getNumBones(); ++i)
    {
      auto bone = skel->getBone(i);

      math::Matrix4d tf(Ogre2Conversions::Convert(bone->getOrientation()));
      tf.SetTranslation(Ogre2Conversions::Convert(bone->getPosition()));

      mapTfs[bone->getName()] = tf;
    }
  }

  return mapTfs;
}

//////////////////////////////////////////////////
void Ogre2Mesh::SetSkeletonLocalTransforms(
          const std::map<std::string, math::Matrix4d> &_tfs)
{
  if (!this->ogreItem->hasSkeleton())
  {
    return;
  }
  auto skel = this->ogreItem->getSkeletonInstance();

  for (auto const &[boneName, tf] : _tfs)
  {
    auto bone = skel->getBone(boneName);
    if (bone)
    {
      skel->setManualBone(bone, true);
      bone->setPosition(Ogre2Conversions::Convert(tf.Translation()));
      bone->setOrientation(Ogre2Conversions::Convert(tf.Rotation()));
    }
  }
}

//////////////////////////////////////////////////
void Ogre2Mesh::SetSkeletonAnimationEnabled(const std::string &_name,
    bool _enabled, bool _loop, float _weight)
{
  if (!this->ogreItem->hasSkeleton())
  {
    return;
  }

  Ogre::SkeletonInstance *skel = this->ogreItem->getSkeletonInstance();

  if (!skel->hasAnimation(_name))
  {
    ignerr << "Skeleton animation name not found: " << _name << std::endl;
    return;
  }

  if (_enabled)
  {
    for (unsigned int i = 0; i < skel->getNumBones(); ++i)
    {
      auto bone = skel->getBone(i);
      skel->setManualBone(bone, false);
    }
  }

  Ogre::SkeletonAnimation *anim = skel->getAnimation(_name);
  anim->setEnabled(_enabled);
  anim->setLoop(_loop);
  anim->mWeight = _weight;
}

//////////////////////////////////////////////////
void Ogre2Mesh::UpdateSkeletonAnimation(double _time)
{
  if (!this->ogreItem->hasSkeleton())
  {
    return;
  }

  Ogre::SkeletonInstance *skel = this->ogreItem->getSkeletonInstance();
  auto animations = skel->getAnimations();
  for (auto &anim : animations)
  {
    Ogre::SkeletonAnimation *sa = skel->getAnimation(anim.getName());
    if (sa->getEnabled())
      sa->setTime(_time);
  }


//  Ogre::Bone *bone = skel->getBone("RightArm");
//  std::cerr << " bone RightArm " <<  bone->getPosition() << " " << bone->getOrientation() << std::endl;;
}

//////////////////////////////////////////////////
bool Ogre2Mesh::SkeletonAnimationEnabled(const std::string &_name) const
{
  if (!this->ogreItem->hasSkeleton())
  {
    return false;
  }

  Ogre::SkeletonInstance *skel = this->ogreItem->getSkeletonInstance();
  if (!skel->hasAnimation(_name))
  {
    ignerr << "Skeleton animation name not found: " << _name << std::endl;
    return false;
  }

  Ogre::SkeletonAnimation *anim = skel->getAnimation(_name);
  return anim->getEnabled();
}

//////////////////////////////////////////////////
Ogre::MovableObject *Ogre2Mesh::OgreObject() const
{
  return this->ogreItem;
}

//////////////////////////////////////////////////
SubMeshStorePtr Ogre2Mesh::SubMeshes() const
{
  return this->subMeshes;
}

//////////////////////////////////////////////////
Ogre2SubMesh::Ogre2SubMesh()
{
}

//////////////////////////////////////////////////
Ogre2SubMesh::~Ogre2SubMesh()
{
  this->Destroy();
}

//////////////////////////////////////////////////
Ogre::SubItem *Ogre2SubMesh::Ogre2SubItem() const
{
  return this->ogreSubItem;
}

//////////////////////////////////////////////////
void Ogre2SubMesh::SetMaterialImpl(MaterialPtr _material)
{
  Ogre2MaterialPtr derived =
      std::dynamic_pointer_cast<Ogre2Material>(_material);

  if (!derived)
  {
    ignerr << "Cannot assign material created by another render-engine"
        << std::endl;

    return;
  }

  this->ogreSubItem->setDatablock(
      static_cast<Ogre::HlmsPbsDatablock *>(derived->Datablock()));

  // set cast shadows
  this->ogreSubItem->getParent()->setCastShadows(_material->CastShadows());
}

//////////////////////////////////////////////////
void Ogre2SubMesh::Init()
{
  BaseSubMesh::Init();
}

//////////////////////////////////////////////////
