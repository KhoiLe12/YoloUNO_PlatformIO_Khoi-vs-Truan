#include "tinyml.h"
#include "temp_humi_monitor.h"  // get_latest_temperature/get_latest_humidity

// Globals, for the convenience of one-shot setup.
namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;
constexpr int kTensorArenaSize = 8 * 1024; // Adjust size based on your model
uint8_t tensor_arena[kTensorArenaSize];
} // namespace

void setupTinyML(){
    Serial.println("TensorFlow Lite Init....");
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(dht_anomaly_model_tflite); // g_model_data is from model_data.h
    if (model->version() != TFLITE_SCHEMA_VERSION) {
    error_reporter->Report("Model provided is schema version %d, not equal to supported version %d.",
                            model->version(), TFLITE_SCHEMA_VERSION);
    return;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
    error_reporter->Report("AllocateTensors() failed");
    return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);


    Serial.println("TensorFlow Lite Micro initialized on ESP32.");
}

void tiny_ml_task(void *pvParameters){
    
    setupTinyML();

    while(1){
       
        // Prepare input data from real sensors
        float t = get_latest_temperature();
        float h = get_latest_humidity();

        // Wait until we have valid readings
        if (isnan(t) || isnan(h)) {
            // Sensor task may not have provided values yet
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        // Optional: clamp to plausible ranges to avoid outliers
        if (t < -40.0f) t = -40.0f; if (t > 125.0f) t = 125.0f;
        if (h < 0.0f)   h = 0.0f;   if (h > 100.0f) h = 100.0f;

        input->data.f[0] = t;
        input->data.f[1] = h;

        // Run inference
        TfLiteStatus invoke_status = interpreter->Invoke();
        if (invoke_status != kTfLiteOk) {
        error_reporter->Report("Invoke failed");
        return;
        }

        // Get and process output
        float result = output->data.f[0];
        // Single atomic print helps avoid fragmented USB CDC output
        Serial.printf("TinyML in: T=%.2fC H=%.1f%%  ->  score=%.3f\r\n", (double)t, (double)h, (double)result);
            Serial.flush();

        vTaskDelay(1000); 
    }
}
