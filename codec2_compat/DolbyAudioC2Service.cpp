#define LOG_TAG "vendor.dolby.media.c2-service-audio"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <codec2/aidl/ComponentStore.h>
#include <C2Component.h>

#include <cstdlib>
#include <memory>
#include <string>

namespace android {
std::shared_ptr<C2ComponentStore> GetCodec2DolbyComponentStore();
}

int main() {
    using ::aidl::android::hardware::media::c2::IComponentStore;
    using ::aidl::android::hardware::media::c2::utils::ComponentStore;

    ABinderProcess_setThreadPoolMaxThreadCount(8);
    ABinderProcess_startThreadPool();

    const std::shared_ptr<C2ComponentStore> componentStore =
            android::GetCodec2DolbyComponentStore();
    if (componentStore == nullptr) {
        LOG(ERROR) << "Dolby Codec2 component store is unavailable";
        return EXIT_FAILURE;
    }

    const auto service = ::ndk::SharedRefBase::make<ComponentStore>(componentStore);
    const std::string serviceName = std::string(IComponentStore::descriptor) + "/dolby_audio";
    const binder_exception_t exception =
            AServiceManager_addService(service->asBinder().get(), serviceName.c_str());
    if (exception != EX_NONE) {
        LOG(ERROR) << "Failed to register " << serviceName << ": " << exception;
        return EXIT_FAILURE;
    }

    LOG(INFO) << "Registered Dolby audio Codec2 service: " << serviceName;
    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;
}
