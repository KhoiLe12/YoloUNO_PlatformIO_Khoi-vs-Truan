from pathlib import Path
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.metrics import precision_recall_fscore_support, confusion_matrix
import numpy as np
import tensorflow as tf

PREFIX = "dht_anomaly_model"
THRESHOLD = 0.05  # Probability threshold for classifying anomaly (Label=1)

# Load dataset (expects header row and semicolon delimiter, trims header spaces)
DATA_PATH = Path(__file__).parent / "dht20_emulated_data_labeled.csv"
data = pd.read_csv(DATA_PATH, sep=';', header=0)
data.columns = data.columns.str.strip()
X = data[["Temperature", "Humidity"]].values
y = data["Label"].values

X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Simple classifier model
model = tf.keras.Sequential([
    tf.keras.layers.Input(shape=(2,)),
    tf.keras.layers.Dense(8, activation='relu'),
    tf.keras.layers.Dense(1, activation='sigmoid')
])

model.compile(loss="binary_crossentropy", optimizer="adam", metrics=["accuracy"])
model.fit(X_train, y_train, epochs=50, validation_data=(X_test, y_test))

# Evaluation: precision, recall, F1 on test set
y_pred_proba = model.predict(X_test).ravel()
y_pred = (y_pred_proba >= THRESHOLD).astype(int)
precision, recall, f1, support = precision_recall_fscore_support(
    y_test, y_pred, average="binary", zero_division=0
)
cm = confusion_matrix(y_test, y_pred)
print("\nEvaluation on test set:")
print(f"  Using threshold: {THRESHOLD:.2f}")
print(f"  Precision: {precision:.4f}")
print(f"  Recall:    {recall:.4f}")
print(f"  F1-score:  {f1:.4f}")
print("  Confusion matrix (rows=true, cols=pred):")
print(cm)

# Threshold sweep to find best F1
def eval_at_threshold(thr: float):
    preds = (y_pred_proba >= thr).astype(int)
    p_b, r_b, f_b, _ = precision_recall_fscore_support(
        y_test, preds, average="binary", zero_division=0
    )
    p_w, r_w, f_w, _ = precision_recall_fscore_support(
        y_test, preds, average="weighted", zero_division=0
    )
    return p_b, r_b, f_b, p_w, r_w, f_w

best_f1, best_thr, best_metrics = -1.0, 0.5, None
for thr in np.linspace(0.05, 0.95, 19):
    p_b, r_b, f_b, p_w, r_w, f_w = eval_at_threshold(float(thr))
    if f_b > best_f1:
        best_f1, best_thr, best_metrics = f_b, float(thr), (p_b, r_b, f_b, p_w, r_w, f_w)

print("\nThreshold sweep (optimize binary F1):")
print(f"  Suggested threshold: {best_thr:.2f}")
print(f"  At {best_thr:.2f} -> Precision: {best_metrics[0]:.4f}, Recall: {best_metrics[1]:.4f}, F1: {best_metrics[2]:.4f}")
print(f"  Weighted scores at {best_thr:.2f} -> Precision: {best_metrics[3]:.4f}, Recall: {best_metrics[4]:.4f}, F1: {best_metrics[5]:.4f}")

model.save(PREFIX + '.h5')


# Convert to TFLite
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
tflite_model = converter.convert()

with open(PREFIX + ".tflite", "wb") as f:
    f.write(tflite_model)

tflite_path = PREFIX + '.tflite'
output_header_path = PREFIX + '.h'

with open(tflite_path, 'rb') as tflite_file:
    tflite_content = tflite_file.read()
model_len = len(tflite_content)

hex_lines = [', '.join([f'0x{byte:02x}' for byte in tflite_content[i:i + 12]]) for i in
         range(0, len(tflite_content), 12)]

hex_array = ',\n  '.join(hex_lines)

with open(output_header_path, 'w') as header_file:
    header_file.write('const unsigned char model[] = {\n  ')
    header_file.write(f'{hex_array}\n')
    header_file.write('};\n\n')
    header_file.write(f'const unsigned int model_len = {model_len};\n')
    header_file.write(f'const float model_threshold = {THRESHOLD:.6f}f;\n')