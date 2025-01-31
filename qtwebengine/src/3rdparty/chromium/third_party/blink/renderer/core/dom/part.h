// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_PART_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_PART_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class Node;
class NodeCloningData;
class PartRoot;
class V8UnionChildNodePartOrDocumentPartRoot;

// Implementation of the Part class, which is part of the DOM Parts API.
class CORE_EXPORT Part : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ~Part() override = default;

  void Trace(Visitor* visitor) const override;
  virtual bool IsValid() const { return root_ && !disconnected_; }
  virtual Node* NodeToSortBy() const = 0;
  virtual Part* ClonePart(NodeCloningData&) const = 0;
  virtual PartRoot* GetAsPartRoot() const { return nullptr; }
  PartRoot* root() const { return root_; }
  void MoveToRoot(PartRoot* new_root);
  virtual Document& GetDocument() const = 0;
  void PartDisconnected(Node& node);
  void PartConnected(Node& node, ContainerNode& insertion_point);

  // Part API
  V8UnionChildNodePartOrDocumentPartRoot* rootForBindings() const;
  const Vector<String>& metadata() const { return metadata_; }
  virtual void disconnect();

 protected:
  Part(PartRoot& root, const Vector<String> metadata);
  bool disconnected_{false};

 private:
  Member<PartRoot> root_;
  Vector<String> metadata_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_PART_H_
