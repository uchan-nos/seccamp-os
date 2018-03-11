#pragma once

#include "usb/error.hpp"
#include "usb/endpoint.hpp"

namespace usb::classdriver
{
  class ClassDriver
  {
    EndpointSet* epset_;
    bool initialized_ = false;
  public:
    ClassDriver(EndpointSet* epset)
      : epset_{epset}
    {}

    EndpointSet* EndpointSet() { return epset_; }
    bool IsInitialized() const { return initialized_; }

    virtual ~ClassDriver() {}
    virtual Error Initialize()
    {
      initialized_ = true;
      return error::kSuccess;
    }

    virtual Error SetEndpoints(EndpointConfig* configs, int len) = 0;
    virtual void OnCompleted(int ep_num) {}
  };
}
