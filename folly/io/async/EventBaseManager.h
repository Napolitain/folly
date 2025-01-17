/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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
 */

#pragma once

#include <memory>

#include <folly/Optional.h>
#include <folly/ThreadLocal.h>
#include <folly/io/async/EventBase.h>

namespace folly {

/**
 * Manager for per-thread EventBase objects.
 *   This class will find or create a EventBase for the current
 *   thread, associated with thread-specific storage for that thread.
 *   Although a typical application will generally only have one
 *   EventBaseManager, there is no restriction on multiple instances;
 *   the EventBases belong to one instance are isolated from those of
 *   another.
 */
class EventBaseManager {
 public:
  /**
   * Constructing a EventBaseManager directly is DEPRECATED and not
   * encouraged. You should instead use the global singleton if possible.
   */
  EventBaseManager() {}

  explicit EventBaseManager(folly::EventBaseBackendBase::FactoryFunc func)
      : func_(func) {}

  ~EventBaseManager() {}

  explicit EventBaseManager(const std::shared_ptr<EventBaseObserver>& observer)
      : observer_(observer) {}

  /**
   * Get the global EventBaseManager for this program. Ideally all users
   * of EventBaseManager go through this interface and do not construct
   * EventBaseManager directly.
   */
  static EventBaseManager* get();

  /**
   * Get the EventBase for this thread, or create one if none exists yet.
   *
   * If no EventBase exists for this thread yet, a new one will be created and
   * returned.  May throw std::bad_alloc if allocation fails.
   */
  EventBase* getEventBase() const;

  /**
   * Get the EventBase for this thread.
   *
   * Returns nullptr if no EventBase has been created for this thread yet.
   */
  EventBase* getExistingEventBase() const {
    if (const auto& info = *localStore_.get()) {
      return info->eventBase;
    }
    return nullptr;
  }

  /**
   * Set the EventBase to be used by this thread.
   *
   * This may only be called if no EventBase has been defined for this thread
   * yet.  If a EventBase is already defined for this thread, a
   * std::runtime_error is thrown.  std::bad_alloc may also be thrown if
   * allocation fails while setting the EventBase.
   *
   * This should typically be invoked by the code that will call loop() on the
   * EventBase, to make sure the EventBaseManager points to the correct
   * EventBase that is actually running in this thread.
   */
  void setEventBase(EventBase* eventBase, bool takeOwnership);

  /**
   * Clear the EventBase for this thread.
   *
   * This can be used if the code driving the EventBase loop() has finished
   * the loop and new events should no longer be added to the EventBase.
   */
  void clearEventBase();

 private:
  struct EventBaseInfo {
    EventBaseInfo(EventBase* evb, bool owned)
        : eventBase(evb), isOwned(owned) {}

    ~EventBaseInfo() {
      if (isOwned) {
        delete eventBase;
      }
    }

    EventBase* eventBase;
    bool isOwned;
  };

  EventBaseManager(EventBaseManager const&) = delete;
  EventBaseManager& operator=(EventBaseManager const&) = delete;

  folly::EventBaseBackendBase::FactoryFunc func_;
  mutable folly::ThreadLocal<folly::Optional<EventBaseInfo>> localStore_;
  std::shared_ptr<folly::EventBaseObserver> observer_;
};

} // namespace folly
