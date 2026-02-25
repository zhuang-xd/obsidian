#ifndef BASE_GLASS_SERVICE_DELEGATE_H_
#define BASE_GLASS_SERVICE_DELEGATE_H_

#include <stdint.h>
#include <string>

class SequenceManager;

class GlassServiceDelegate {
public:
    virtual ~GlassServiceDelegate() = default;

public:
    virtual void OnTakePhoto() = 0;
    virtual void OnStartRecording() = 0;
    virtual void OnStopRecording() = 0;
    virtual void OnStartWebService() = 0;
    virtual bool OnOTAStartup(int device, const char* url) = 0;
    virtual void OnLinuxUpgraded(int result) = 0;
    virtual void OnARCHUpgraded(int result) = 0;
    virtual void OnOTAProgress(int stage, int percentage, const char* message) = 0;
    virtual void OnRequestFileList(uint16_t* file_count, std::string& thumbnail_path) = 0;
    virtual void OnCommandStart(void) = 0;
    virtual void OnCommandEnd(bool shutdown = true) = 0;
    virtual void OnBeforePoweroff(void) = 0;
    virtual void OnAfterOTA(int fd, int type, int result) = 0;
    virtual void PoweroffForce() = 0;
    virtual bool IsTransfering(void) = 0;
    virtual void SetP2PName(const std::string& name) = 0;
    virtual SequenceManager* GetSequenceManager(void) = 0;


};

#endif  // BASE_GLASS_SERVICE_DELEGATE_H_
