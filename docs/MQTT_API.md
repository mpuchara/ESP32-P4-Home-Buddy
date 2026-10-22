# MQTT status

Removed from the current firmware at the operator request on2026-10-22.
Temperature now comes from Open-Meteo with manual locality selection; see
[weather API](WEATHER_API.md). No MQTT menu, client, subscription or temperature
payload parser is active or linked. No broker credentials are required.

The initial specification's future Home Assistant identity/discovery/event design
remains unimplemented. It is not evidence of an MQTT feature in0.2.1-dev.
