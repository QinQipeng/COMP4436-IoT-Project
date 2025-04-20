import pandas as pd
import matplotlib.pyplot as plt
from sklearn.preprocessing import LabelEncoder
from sklearn.tree import DecisionTreeRegressor, plot_tree
from sklearn.model_selection import train_test_split
from sklearn.metrics import mean_squared_error

# Load data
soil_data = pd.read_csv('soil_data.csv')
light_data = pd.read_csv('light_data.csv')

# Merge data based on time
soil_data['time'] = pd.to_datetime(soil_data['time'])
light_data['time'] = pd.to_datetime(light_data['time'])
data = pd.merge(soil_data, light_data, on='time', how='inner')

# Encode the soil status as numerical for regression
label_encoder = LabelEncoder()
data['soil_status_encoded'] = label_encoder.fit_transform(data['soil status'])

# Features and target variable
X = data[['light', 'temperature', 'humidity', 'soil moisture']]
y = data['soil_status_encoded']

# Split the data into training and testing sets
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Train the Decision Tree Regressor
model = DecisionTreeRegressor(random_state=42)
model.fit(X_train, y_train)

# Evaluate the model
y_pred = model.predict(X_test)
print("Mean Squared Error:", mean_squared_error(y_test, y_pred))

# Plot the trained Decision Tree
plt.figure(figsize=(20, 10))
plot_tree(model, feature_names=X.columns, filled=True, fontsize=10)
plt.title("Decision Tree Regressor", fontsize=16)
plt.show()

# Live prediction function
def should_water_now(live_data):
    live_df = pd.DataFrame([live_data])
    prediction = model.predict(live_df)

    # Map prediction back to soil status
    soil_status = label_encoder.inverse_transform([round(prediction[0])])[0]
    return soil_status == "Dry"

# Example live data
# Dry
live_data = {
    'light': 2200,
    'temperature': 28,
    'humidity': 80,
    'soil moisture' : 4000
}
# Wet
# live_data = {
#     'light': 2200,
#     'temperature': 28,
#     'humidity': 80,
#     'soil moisture' : 2000
# }

# Check if watering is needed
if should_water_now(live_data):
    print("Watering is needed now.")
else:
    print("No watering needed.")