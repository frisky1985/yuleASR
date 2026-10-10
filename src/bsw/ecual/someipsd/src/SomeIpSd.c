/*==================================================================================================
* Duplicate-module convergence: tombstone for ecual/someipsd
*
* Canonical implementation: services/someip (SomeIpSd.c)
*   - More complete API: 5-param OfferService, typed config, callbacks
*   - 357 LOC vs 274 LOC
*   - Richer SD message/entry/option type definitions
*
* Deferred unique APIs from this (ecual) side:
*   - SomeIpSd_GetServiceState()   -- state query
*   - SomeIpSd_StopOffer()         -- 2-param stop offer
*   - SomeIpSd_SubscribeEventGroup() -- 2-param subscribe
*   These were NOT merged into the canonical side because no BSW consumer
*   calls them. If needed in the future, merge into services/someip.
*
* Include path preserved: ecual/someipsd/include/ remains accessible via
* the CMake shim target (ecual_someipsd) so any legacy #include paths
* continue to resolve.
*================================================================================================*/
