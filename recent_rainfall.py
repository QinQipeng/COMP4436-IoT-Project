import requests

def get_hourly_rainfall(target_station="Tuen Mun"):
    url = "https://data.weather.gov.hk/weatherAPI/opendata/hourlyRainfall.php?lang=en"
    response = requests.get(url)

    if response.status_code == 200:
        data = response.json()
        print(f"Observation Time: {data['obsTime']}")

        rainfall_list = data.get("hourlyRainfall", [])
        for record in rainfall_list:
            station = record.get("automaticWeatherStation")
            value = record.get("value")
            unit = record.get("unit")

            # Optional: Print all stations' rainfall
            print(f"{station}: {value} {unit}")

            if station == target_station:
                return {
                    "station": station,
                    "rainfall": value,
                    "unit": unit,
                    "obversationTime": data["obsTime"]
                }

        print(f"Station '{target_station}' not found in data.")
        return None
    else:
        print("❌ Failed to fetch data.")
        return None

# Example usage
rainfall_data = get_hourly_rainfall()
if rainfall_data:
    print("\n📊 Recent Rainfall:")
    print(rainfall_data)
