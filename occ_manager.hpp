#pragma once

#include "occ_pass_through.hpp"
#include "occ_status.hpp"
#ifdef PLDM
#include "pldm.hpp"
#endif
#include "powercap.hpp"

#include <cstring>
#include <functional>
#include <sdbusplus/bus.hpp>
#include <vector>

namespace sdbusRule = sdbusplus::bus::match::rules;
namespace open_power
{
namespace occ
{

/** @class Manager
 *  @brief Builds and manages OCC objects
 */
struct Manager
{
  public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;
    ~Manager() = default;

    /** @brief Adds OCC pass-through and status objects on the bus
     *         when corresponding CPU inventory is created.
     *
     *  @param[in] bus   - handle to the bus
     *  @param[in] event - Unique ptr reference to sd_event
     */
    Manager(sdbusplus::bus::bus& bus, EventPtr& event) :
        bus(bus), event(event)
#ifdef PLDM
        ,
        pldmHandle(std::make_unique<pldm::Interface>(
            bus, std::bind(std::mem_fn(&Manager::updateOCCActive), this,
                           std::placeholders::_1, std::placeholders::_2)))
#endif

    {
#ifdef I2C_OCC
        // I2C OCC status objects are initialized directly
        initStatusObjects();
#else
        findAndCreateObjects();
#endif
    }

    inline auto getNumOCCs() const
    {
        return activeCount;
    }

  private:
    /** @brief Checks if the CPU inventory is present and if so, creates
     *         the occ D-Bus objects. Else, registers a handler to be
     *         called when inventory is created.
     */
    void findAndCreateObjects();

    /** @brief Callback that responds to cpu creation in the inventory -
     *         by creating the needed objects.
     *
     *  @param[in] msg - bus message
     *
     *  @returns 0 to indicate success
     */
    int cpuCreated(sdbusplus::message::message& msg);

    /** @brief Create child OCC objects.
     *
     *  @param[in] occ - the occ name, such as occ0.
     */
    void createObjects(const std::string& occ);

    /** @brief Callback handler invoked by Status object when the OccActive
     *         property is changed. This is needed to make sure that the
     *         error detection is started only after all the OCCs are bound.
     *         Similarly, when one of the OCC gets its OccActive property
     *         un-set, then the OCC error detection needs to be stopped on
     *         all the OCCs
     *
     *  @param[in] status - OccActive status
     */
    void statusCallBack(bool status);

    /** @brief Sends a Heartbeat command to host control command handler */
    void sendHeartBeat();

    /** @brief reference to the bus */
    sdbusplus::bus::bus& bus;

    /** @brief reference to sd_event wrapped in unique_ptr */
    EventPtr& event;

    /** @brief OCC pass-through objects */
    std::vector<std::unique_ptr<PassThrough>> passThroughObjects;

    /** @brief OCC Status objects */
    std::vector<std::unique_ptr<Status>> statusObjects;

    /** @brief Power cap monitor and occ notification object */
    std::unique_ptr<open_power::occ::powercap::PowerCap> pcap;

    /** @brief sbdbusplus match objects */
    std::vector<sdbusplus::bus::match_t> cpuMatches;

    /** @brief Number of OCCs that are bound */
    uint8_t activeCount = 0;

#ifdef I2C_OCC
    /** @brief Init Status objects for I2C OCC devices
     *
     * It iterates in /sys/bus/i2c/devices, finds all occ hwmon devices
     * and creates status objects.
     */
    void initStatusObjects();
#endif

#ifdef PLDM
    /** @brief Callback handler invoked by the PLDM event handler when state of
     *         the OCC is toggled by the host. The caller passes the instance
     *         of the OCC and state of the OCC.
     *
     *  @param[in] instance - instance of the OCC
     *  @param[in] status - true when the OCC goes active and false when the OCC
     *                      goes inactive
     *
     *  @return true if setting the state of OCC is successful and false if it
     *          fails.
     */
    bool updateOCCActive(instanceID instance, bool status);

    std::unique_ptr<pldm::Interface> pldmHandle = nullptr;
#endif
};

} // namespace occ
} // namespace open_power
