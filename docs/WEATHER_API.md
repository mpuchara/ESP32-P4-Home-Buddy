# Open-Meteo temperature,0.2.1-dev

The operator replaced MQTT temperature with Open-Meteo and manual locality entry.
MQTT client/UI/subscription and its old payload parser were removed. No broker is
contacted. The old schema-1 transport area is opaque compatibility padding,
cleared in RAM and on the next general-settings save. Wi-Fi/UBox credentials and
the NVS record's852/900-byte layouts remain compatible (compile-time assertions).
Location uses a separate versioned weather record and does not overwrite them.

## Selection and source

Pogoda -> type locality -> Szukaj -> choose country/region -> Zapisz. Up to5results
prevent silently assigning an ambiguous first match. Changed text invalidates a
prior selection; results for a previous query are ignored. Empty city disables
weather. No city/IP geolocation is guessed. ASCII spelling works with the service's
case/diacritic-insensitive search.

Geocoding endpoint, percent-encoded UTF-8 query, Polish display language:

```text
https://geocoding-api.open-meteo.com/v1/search?name=<encoded>&count=5&language=pl&format=json
```

Current endpoint uses the selected coordinates, Celsius and UTC Unix seconds:

```text
https://api.open-meteo.com/v1/forecast?latitude=<lat>&longitude=<lon>&current=temperature_2m&temperature_unit=celsius&timeformat=unixtime&timezone=GMT&forecast_days=1
```

Poll every600seconds; errors back off5..300seconds. Require synchronized wall time,
finite numeric Celsius within-80..80, integer Unix timestamp, exact Celsius unit,
no future timestamp and age<=3600seconds. Preserve the old value only until its
original timestamp expires; show unavailable thereafter. Selecting another location
clears the previous locality's temperature. A download finishing after a location
change cannot publish the old location's value.

TLS validates the hostname/certificate using the IDF CA bundle. Redirects are
rejected. Responses cap at8192bytes, nesting at8, timeouts8seconds per call and
20seconds total. Search/HTTP/parsing are off the LVGL task. A single pending search
is bounded and replaceable. Search-result commands copy bounded data with explicit
heap ownership; stale/page-closed results are freed without UI access.

These current conditions are derived from weather-model data, not a measurement
from a household sensor. Attribution Open-Meteo.com is shown in settings. Use the
service under its applicable licensing/usage terms; this integration uses the
public personal/development endpoints and has no commercial customer-key flow.

Sources inspected2026-10-22:
[Weather API](https://open-meteo.com/en/docs),
[Geocoding API](https://open-meteo.com/en/docs/geocoding-api).

## Validation

Host tests execute actual cJSON parsing for locations, UTF-8 query encoding,
coordinates, missing/null/wrong-unit/string/invalid/stale/future data and depth.
Public Warsaw API responses(1087/319bytes) were downloaded with certificate
verification and parsed by the same C functions. Warsaw is a contract probe,
not a default or the operator's configured location. The operator subsequently confirmed device search, selection, saving and a
temperature displayed on the saver. Long outage/staleness/fault/soak acceptance
remains open. The host probe is separate evidence from that operator confirmation.
