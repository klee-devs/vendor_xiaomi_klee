
#include <algorithm>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <log/log.h>

#include <C2IgbaInterfaceImpl.h>
#include <C2PlatformSupport.h>
#include <aidl/android/hardware/media/c2/IGraphicBufferAllocator.h>
#include <codec2/common/MultiAccessUnitHelper.h>

#include "FilterWrapper.h"

namespace android {

c2_status_t CreateCodec2BlockPool(
        C2PlatformAllocatorDesc &allocator,
        const std::vector<std::shared_ptr<const C2Component>> &components,
        std::shared_ptr<C2BlockPool> *pool) {
    if (pool == nullptr) {
        return C2_BAD_VALUE;
    }

    pool->reset();

    C2PlatformAllocatorDescV2 allocatorV2;
    allocatorV2.allocatorId = allocator.allocatorId;
    if (allocator.igba) {
        allocatorV2.igba = std::make_shared<C2IgbaInterfaceImpl>(allocator.igba);
        allocatorV2.waitableFd = std::move(allocator.waitableFd);
    }

    return CreateCodec2BlockPool(allocatorV2, components, pool);
}

c2_status_t CreateCodec2BlockPool(
        C2PlatformAllocatorDesc &allocator,
        std::shared_ptr<const C2Component> component,
        std::shared_ptr<C2BlockPool> *pool) {
    return CreateCodec2BlockPool(
            allocator,
            std::vector<std::shared_ptr<const C2Component>>{std::move(component)},
            pool);
}

c2_status_t FilterWrapper::createBlockPool(
        C2PlatformAllocatorDesc &allocatorParam,
        std::shared_ptr<const C2Component> component,
        std::shared_ptr<C2BlockPool> *pool) {
    std::unique_lock lock(mWrappedComponentsMutex);
    for (auto it = mWrappedComponents.begin(); it != mWrappedComponents.end();) {
        std::shared_ptr<const C2Component> wrapped = it->front().lock();
        if (!wrapped) {
            it = mWrappedComponents.erase(it);
            continue;
        }
        if (component == wrapped) {
            std::vector<std::shared_ptr<const C2Component>> components(it->size());
            std::transform(
                    it->begin(), it->end(), components.begin(),
                    [](const std::weak_ptr<const C2Component> &entry) {
                        return entry.lock();
                    });
            if (C2_OK == CreateCodec2BlockPool(allocatorParam, components, pool)) {
                return C2_OK;
            }
        }
        ++it;
    }
    return CreateCodec2BlockPool(allocatorParam, std::move(component), pool);
}

bool MultiAccessUnitHelper::isEnabledOnPlatform() {
    // Android 17 removed the legacy large_audio_frame flag. Keep its old
    // default (disabled) so vendor callers use single-access-unit handling.
    return false;
}

}  // namespace android
