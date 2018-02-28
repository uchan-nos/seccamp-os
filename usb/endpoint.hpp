#pragma once

#include "usb/error.hpp"

namespace usb
{
  class EndpointSet
  {
  public:
    virtual ~EndpointSet() {}

    /** @brief ControlOut issues an OUT control transfer.
     *
     * @param ep_num  A control endpoint.
     * @param setup_data  A composite of bmRequestType, bRequest, wValue,
     *  wIndex, wLength. (bmRequestType is 0-7 bit, wLength is 48-63 bit)
     * @param buf  Data buffer. nullptr if no data stage.
     * @param len  The number of bytes of the buffer.
     * @return error code.
     */
    virtual Error ControlOut(int ep_num, uint64_t setup_data,
                             const void* buf, int len) = 0;

    /** @brief ControlIn issues an IN control transfer.
     *
     * @param ep_num  A control endpoint.
     * @param setup_data  A composite of bmRequestType, bRequest, wValue,
     *  wIndex, wLength. (bmRequestType is 0-7 bit, wLength is 48-63 bit)
     * @param buf  Data buffer. nullptr if no data stage.
     * @param len  The number of bytes of the buffer.
     * @return error code.
     */
    virtual Error ControlIn(int ep_num, uint64_t setup_data,
                            void* buf, int len) = 0;

    /** @brief Send schedules to send data to the specified endpoint.
     *
     * @param ep_num  An endpoint to which the data will be sent. (0 - 15)
     * @param buf  Data buffer.
     * @param len  The number of bytes of the buffer.
     * @return error code.
     */
    virtual Error Send(int ep_num, const void* buf, int len) = 0;

    /** @brief Receive schedules to receive data from the specified endpoint.
     * This method shall return just after scheduling the receipt.
     * Data will be received asynchronously and a completion event will occur.
     *
     * @param ep_num  An endpoint from which the data will be received. (0 - 15)
     * @param buf  Data buffer.
     * @param len  The number of bytes of the buffer.
     * @return error code.
     */
    virtual Error Receive(int ep_num, void* buf, int len) = 0;
  };
}
