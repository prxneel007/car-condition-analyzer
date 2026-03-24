#include <gtk/gtk.h>
#include <pango/pango.h>  
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>


#define TEMP_LOW 60
#define TEMP_HIGH 120
#define BATTERY_VOLTAGE_MIN 11.5
#define BATTERY_AGE_MAX 5
#define NUM_TIRES 4
#define TIRE_PRESSURE_MIN 28
#define TIRE_PRESSURE_MAX 36
#define PRESSURE_DIFF_THRESHOLD 3.0
#define OIL_MONTHS_MAX 6
#define OIL_LEVEL_MIN 70
#define COOLANT_LEVEL_MIN 60
#define BRAKE_FLUID_MIN 60
#define TRANSMISSION_FLUID_MIN 70

// Color codes for output
#define RED "<span foreground=\"red\">"
#define GREEN "<span foreground=\"green\">"
#define YELLOW "<span foreground=\"orange\">"
#define RESET "</span>"

// Structure to store tire status
typedef struct {
    float pressures[NUM_TIRES];
    int warnings[NUM_TIRES];
    int imbalance;
} TireStatus;

// Structure to store car diagnostic data
typedef struct {
    float engineTemp;
    int engineNoiseFlag;
    float batteryVoltage;
    int batteryAge;
    int frontBrakeWear;
    int rearBrakeWear;
    float tirePressures[NUM_TIRES];
    int oilMonthsSinceChange;
    int oilLevel;
    int coolantLevel;
    int brakeFluidLevel;
    int transmissionFluidLevel;
} CarStatus;

// Structure to hold all UI widgets
typedef struct {
    GtkWidget *window;
    GtkWidget *diagnose_button;
    GtkWidget *entry_engine_temp;
    GtkWidget *check_engine_noise;
    GtkWidget *entry_battery_voltage;
    GtkWidget *entry_battery_age;
    GtkWidget *entry_front_brake;
    GtkWidget *entry_rear_brake;
    GtkWidget *entry_tire_pressures[NUM_TIRES];
    GtkWidget *entry_oil_months;
    GtkWidget *entry_oil_level;
    GtkWidget *entry_coolant_level;
    GtkWidget *entry_brake_fluid;
    GtkWidget *entry_trans_fluid;
    GtkWidget *text_view;
    GtkWidget *simulate_check;
} AppWidgets;

// Function prototypes
void create_main_window(GtkApplication *app, AppWidgets *widgets);
void append_to_text_view(GtkWidget *text_view, const char *text);
void clear_text_view(GtkWidget *text_view);
int validate_inputs(AppWidgets *widgets, CarStatus *car);
void simulate_inputs(AppWidgets *widgets);
void on_simulate_toggled(GtkCheckButton *button, AppWidgets *widgets);
void on_input_changed(GtkEditable *editable, AppWidgets *widgets);
void on_diagnose_clicked(GtkButton *button, AppWidgets *widgets);
void escape_pango_markup(const char *input, char *output, size_t output_size);
const char* checkEngineTemp(float temp, int* score, int* status);
const char* checkEngineNoise(int noiseFlag, int* score, int* status);
const char* checkBatteryVoltage(float voltage, int* score, int* status);
const char* checkBatteryAge(int ageYears, int* score, int* status);
const char* evaluateBrakePad(const char* label, int wearPercent, int* score, int* status);
TireStatus checkTirePressure(float pressures[NUM_TIRES]);
int validatePressure(float p);
void generateTirePressureReport(TireStatus tireStatus, int* score, char* report);
const char* checkOilChangeStatus(int months, int* score, int* status);
const char* checkOilLevel(int level, int* score, int* status);
const char* checkCoolantLevel(int level, int* score, int* status);
const char* checkBrakeFluidLevel(int level, int* score, int* status);
const char* checkTransmissionFluidLevel(int level, int* score, int* status);
void displayScoreBar(int score, char* buffer);
void showHealthTip(int score, char* buffer, int* status);
void sendSOS(const char* reason);
void saveCSV(const char* report);

// Utility function to escape Pango markup characters
void escape_pango_markup(const char *input, char *output, size_t output_size) {
    if (!input || !output) return;
    int j = 0;
    for (int i = 0; input[i] != '\0' && j < output_size - 6; i++) {
        if (input[i] == '&') {
            strcpy(&output[j], "&");
            j += 5;
        } else if (input[i] == '<') {
            strcpy(&output[j], "<");
            j += 4;
        } else if (input[i] == '>') {
            strcpy(&output[j], ">");
            j += 4;
        } else if (input[i] == '"') {
            strcpy(&output[j], "");
            j += 6;
        } else {
            output[j++] = input[i];
        }
    }
    output[j] = '\0';
}

// Strip markup tags to create plain text (for fallback)
void strip_markup(const char *input, char *output, size_t output_size) {
    if (!input || !output) return;
    int j = 0;
    int in_tag = 0;
    for (int i = 0; input[i] != '\0' && j < output_size - 1; i++) {
        if (input[i] == '<') {
            in_tag = 1;
            continue;
        }
        if (input[i] == '>' && in_tag) {
            in_tag = 0;
            continue;
        }
        if (!in_tag) {
            output[j++] = input[i];
        }
    }
    output[j] = '\0';
}

// Append text to text view with markup validation
void append_to_text_view(GtkWidget *text_view, const char *text) {
    if (!text_view || !text) return;

    // Step 1: Test if the GtkTextView can display plain text
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    if (!buffer) {
        printf("Error: Could not get text buffer from text view\n");
        return;
    }
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, "Testing plain text display...\n", -1);

    // Step 2: Validate the markup using pango_parse_markup
    GError *error = NULL;
    PangoAttrList *attr_list = NULL;
    char *parsed_text = NULL;
    if (!pango_parse_markup(text, -1, 0, &attr_list, &parsed_text, NULL, &error)) {
        printf("Pango Markup Error: %s\n", error->message);
        g_error_free(error);

        // Fallback: Strip markup and insert plain text
        char plain_text[10000];
        strip_markup(text, plain_text, sizeof(plain_text));
        gtk_text_buffer_get_end_iter(buffer, &end);
        gtk_text_buffer_insert(buffer, &end, plain_text, -1);
    } else {
        // Markup is valid, insert it
        gtk_text_buffer_get_end_iter(buffer, &end);
        gtk_text_buffer_insert_markup(buffer, &end, text, -1);
    }

    // Clean up
    if (attr_list) pango_attr_list_unref(attr_list);
    if (parsed_text) g_free(parsed_text);
}

// Clear text view
void clear_text_view(GtkWidget *text_view) {
    if (!text_view) return;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    if (!buffer) return;
    gtk_text_buffer_set_text(buffer, "", -1);
}

// Validate inputs
int validate_inputs(AppWidgets *widgets, CarStatus *car) {
    if (!widgets || !car) return 0;
    char *endptr;
    const char *text;

    // Engine & Battery
    text = gtk_editable_get_text(GTK_EDITABLE(widgets->entry_engine_temp));
    if (!text || *text == '\0') return 0;
    car->engineTemp = strtof(text, &endptr);
    if (*endptr != '\0') return 0;

    car->engineNoiseFlag = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->check_engine_noise));

    text = gtk_editable_get_text(GTK_EDITABLE(widgets->entry_battery_voltage));
    if (!text || *text == '\0') return 0;
    car->batteryVoltage = strtof(text, &endptr);
    if (*endptr != '\0') return 0;

    text = gtk_editable_get_text(GTK_EDITABLE(widgets->entry_battery_age));
    if (!text || *text == '\0') return 0;
    car->batteryAge = strtol(text, &endptr, 10);
    if (*endptr != '\0') return 0;

    // Brakes
    text = gtk_editable_get_text(GTK_EDITABLE(widgets->entry_front_brake));
    if (!text || *text == '\0') return 0;
    car->frontBrakeWear = strtol(text, &endptr, 10);
    if (*endptr != '\0') return 0;

    text = gtk_editable_get_text(GTK_EDITABLE(widgets->entry_rear_brake));
    if (!text || *text == '\0') return 0;
    car->rearBrakeWear = strtol(text, &endptr, 10);
    if (*endptr != '\0') return 0;

    // Tires
    for (int i = 0; i < NUM_TIRES; i++) {
        text = gtk_editable_get_text(GTK_EDITABLE(widgets->entry_tire_pressures[i]));
        if (!text || *text == '\0') return 0;
        car->tirePressures[i] = strtof(text, &endptr);
        if (*endptr != '\0' || !validatePressure(car->tirePressures[i])) {
            char message[1000];
            escape_pango_markup("Invalid tire pressure. Must be between 0-50 PSI.\n", message, sizeof(message));
            char formatted[1100];
            snprintf(formatted, sizeof(formatted), "%s%s%s", RED, message, RESET);
            append_to_text_view(widgets->text_view, formatted);
            return 0;
        }
    }

    // Fluids
    text = gtk_editable_get_text(GTK_EDITABLE(widgets->entry_oil_months));
    if (!text || *text == '\0') return 0;
    car->oilMonthsSinceChange = strtol(text, &endptr, 10);
    if (*endptr != '\0') return 0;

    text = gtk_editable_get_text(GTK_EDITABLE(widgets->entry_oil_level));
    if (!text || *text == '\0') return 0;
    car->oilLevel = strtol(text, &endptr, 10);
    if (*endptr != '\0') return 0;

    text = gtk_editable_get_text(GTK_EDITABLE(widgets->entry_coolant_level));
    if (!text || *text == '\0') return 0;
    car->coolantLevel = strtol(text, &endptr, 10);
    if (*endptr != '\0') return 0;

    text = gtk_editable_get_text(GTK_EDITABLE(widgets->entry_brake_fluid));
    if (!text || *text == '\0') return 0;
    car->brakeFluidLevel = strtol(text, &endptr, 10);
    if (*endptr != '\0') return 0;

    text = gtk_editable_get_text(GTK_EDITABLE(widgets->entry_trans_fluid));
    if (!text || *text == '\0') return 0;
    car->transmissionFluidLevel = strtol(text, &endptr, 10);
    if (*endptr != '\0') return 0;

    return 1;
}

// Simulate inputs
void simulate_inputs(AppWidgets *widgets) {
    if (!widgets) return;
    srand(time(NULL));
    char buffer[100];

    snprintf(buffer, sizeof(buffer), "%.1f", (float)(60 + rand() % 61));  // 60-120°C
    gtk_editable_set_text(GTK_EDITABLE(widgets->entry_engine_temp), buffer);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->check_engine_noise), rand() % 2);
    snprintf(buffer, sizeof(buffer), "%.1f", 11.0 + (rand() % 20) / 10.0);  // 11.0-13.0V
    gtk_editable_set_text(GTK_EDITABLE(widgets->entry_battery_voltage), buffer);
    snprintf(buffer, sizeof(buffer), "%d", rand() % 7);  // 0-6 years
    gtk_editable_set_text(GTK_EDITABLE(widgets->entry_battery_age), buffer);

    snprintf(buffer, sizeof(buffer), "%d", rand() % 101);  // 0-100%
    gtk_editable_set_text(GTK_EDITABLE(widgets->entry_front_brake), buffer);
    snprintf(buffer, sizeof(buffer), "%d", rand() % 101);  // 0-100%
    gtk_editable_set_text(GTK_EDITABLE(widgets->entry_rear_brake), buffer);

    for (int i = 0; i < NUM_TIRES; i++) {
        snprintf(buffer, sizeof(buffer), "%.1f", 28.0 + (rand() % 10));  // 28-38 PSI
        gtk_editable_set_text(GTK_EDITABLE(widgets->entry_tire_pressures[i]), buffer);
    }

    snprintf(buffer, sizeof(buffer), "%d", rand() % 13);  // 0-12 months
    gtk_editable_set_text(GTK_EDITABLE(widgets->entry_oil_months), buffer);
    snprintf(buffer, sizeof(buffer), "%d", 50 + rand() % 51);  // 50-100%
    gtk_editable_set_text(GTK_EDITABLE(widgets->entry_oil_level), buffer);
    snprintf(buffer, sizeof(buffer), "%d", 50 + rand() % 51);  // 50-100%
    gtk_editable_set_text(GTK_EDITABLE(widgets->entry_coolant_level), buffer);
    snprintf(buffer, sizeof(buffer), "%d", 50 + rand() % 51);  // 50-100%
    gtk_editable_set_text(GTK_EDITABLE(widgets->entry_brake_fluid), buffer);
    snprintf(buffer, sizeof(buffer), "%d", 50 + rand() % 51);  // 50-100%
    gtk_editable_set_text(GTK_EDITABLE(widgets->entry_trans_fluid), buffer);
}

// Callback for simulate button
void on_simulate_toggled(GtkCheckButton *button, AppWidgets *widgets) {
    if (!widgets) return;
    if (gtk_check_button_get_active(button)) {
        simulate_inputs(widgets);
    }
}

// Callback for input field changes
void on_input_changed(GtkEditable *editable, AppWidgets *widgets) {
    if (!widgets) return;

    if (!gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->simulate_check))) {
        if (gtk_entry_get_text_length(GTK_ENTRY(widgets->entry_engine_temp)) > 0 &&
            gtk_entry_get_text_length(GTK_ENTRY(widgets->entry_battery_voltage)) > 0 &&
            gtk_entry_get_text_length(GTK_ENTRY(widgets->entry_battery_age)) > 0 &&
            gtk_entry_get_text_length(GTK_ENTRY(widgets->entry_front_brake)) > 0 &&
            gtk_entry_get_text_length(GTK_ENTRY(widgets->entry_rear_brake)) > 0 &&
            gtk_entry_get_text_length(GTK_ENTRY(widgets->entry_tire_pressures[0])) > 0 &&
            gtk_entry_get_text_length(GTK_ENTRY(widgets->entry_tire_pressures[1])) > 0 &&
            gtk_entry_get_text_length(GTK_ENTRY(widgets->entry_tire_pressures[2])) > 0 &&
            gtk_entry_get_text_length(GTK_ENTRY(widgets->entry_tire_pressures[3])) > 0 &&
            gtk_entry_get_text_length(GTK_ENTRY(widgets->entry_oil_months)) > 0 &&
            gtk_entry_get_text_length(GTK_ENTRY(widgets->entry_oil_level)) > 0 &&
            gtk_entry_get_text_length(GTK_ENTRY(widgets->entry_coolant_level)) > 0 &&
            gtk_entry_get_text_length(GTK_ENTRY(widgets->entry_brake_fluid)) > 0 &&
            gtk_entry_get_text_length(GTK_ENTRY(widgets->entry_trans_fluid)) > 0) {
            g_signal_emit_by_name(widgets->diagnose_button, "clicked");
        }
    }
}

// Callback for dialog response
static void on_save_dialog_response(GtkButton *button, gpointer user_data) {
    AppWidgets *widgets = (AppWidgets *)user_data;
    if (g_object_get_data(G_OBJECT(button), "response") == GINT_TO_POINTER(GTK_RESPONSE_YES)) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets->text_view));
        GtkTextIter start, end;
        gtk_text_buffer_get_start_iter(buffer, &start);
        gtk_text_buffer_get_end_iter(buffer, &end);
        char *report_text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
        saveCSV(report_text);
        g_free(report_text);
        char message[1000];
        escape_pango_markup("Saved to vehicle_report.csv\n", message, sizeof(message));
        char formatted[1100];
        snprintf(formatted, sizeof(formatted), "%s%s%s", GREEN, message, RESET);
        append_to_text_view(widgets->text_view, formatted);
    }
    gtk_window_destroy(GTK_WINDOW(gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_WINDOW)));
}

// Callback for diagnose button
void on_diagnose_clicked(GtkButton *button, AppWidgets *widgets) {
    if (!widgets) return;
    CarStatus car;
    int score = 100;
    char report[10000] = "";
    char temp[500] = "";
    char escaped[1000] = "";
    int status;

    clear_text_view(widgets->text_view);

    if (!validate_inputs(widgets, &car)) {
        char message[1000];
        escape_pango_markup("Invalid input. Enter valid numbers.\n", message, sizeof(message));
        char formatted[1100];
        snprintf(formatted, sizeof(formatted), "%s%s%s", RED, message, RESET);
        append_to_text_view(widgets->text_view, formatted);
        return;
    }

    // Generate report
    escape_pango_markup("Vehicle Health Report\n-----------------\n", temp, sizeof(temp));
    strcat(report, temp);

    // Engine & Battery
    escape_pango_markup("Engine & Battery:\n", temp, sizeof(temp));
    strcat(report, temp);

    const char *msg = checkEngineTemp(car.engineTemp, &score, &status);
    escape_pango_markup(msg, escaped, sizeof(escaped));
    snprintf(temp, sizeof(temp), "%s%s%s\n", status == 0 ? GREEN : status == 1 ? RED : YELLOW, escaped, RESET);
    strcat(report, temp);

    msg = checkEngineNoise(car.engineNoiseFlag, &score, &status);
    escape_pango_markup(msg, escaped, sizeof(escaped));
    snprintf(temp, sizeof(temp), "%s%s%s\n", status == 0 ? GREEN : RED, escaped, RESET);
    strcat(report, temp);

    msg = checkBatteryVoltage(car.batteryVoltage, &score, &status);
    escape_pango_markup(msg, escaped, sizeof(escaped));
    snprintf(temp, sizeof(temp), "%s%s%s\n", status == 0 ? GREEN : RED, escaped, RESET);
    strcat(report, temp);

    msg = checkBatteryAge(car.batteryAge, &score, &status);
    escape_pango_markup(msg, escaped, sizeof(escaped));
    snprintf(temp, sizeof(temp), "%s%s%s\n", status == 0 ? GREEN : RED, escaped, RESET);
    strcat(report, temp);

    // Brakes
    escape_pango_markup("Brakes:\n", temp, sizeof(temp));
    strcat(report, temp);

    msg = evaluateBrakePad("Front", car.frontBrakeWear, &score, &status);
    escape_pango_markup(msg, escaped, sizeof(escaped));
    snprintf(temp, sizeof(temp), "%s%s%s\n", status == 0 ? GREEN : status == 1 ? RED : YELLOW, escaped, RESET);
    strcat(report, temp);

    msg = evaluateBrakePad("Rear", car.rearBrakeWear, &score, &status);
    escape_pango_markup(msg, escaped, sizeof(escaped));
    snprintf(temp, sizeof(temp), "%s%s%s\n", status == 0 ? GREEN : status == 1 ? RED : YELLOW, escaped, RESET);
    strcat(report, temp);

    // Tires
    escape_pango_markup("Tires:\n", temp, sizeof(temp));
    strcat(report, temp);
    TireStatus tireStatus = checkTirePressure(car.tirePressures);
    generateTirePressureReport(tireStatus, &score, report);

    // Fluids
    escape_pango_markup("Fluids:\n", temp, sizeof(temp));
    strcat(report, temp);

    msg = checkOilChangeStatus(car.oilMonthsSinceChange, &score, &status);
    escape_pango_markup(msg, escaped, sizeof(escaped));
    snprintf(temp, sizeof(temp), "%s%s%s\n", status == 0 ? GREEN : RED, escaped, RESET);
    strcat(report, temp);

    msg = checkOilLevel(car.oilLevel, &score, &status);
    escape_pango_markup(msg, escaped, sizeof(escaped));
    snprintf(temp, sizeof(temp), "%s%s%s\n", status == 0 ? GREEN : RED, escaped, RESET);
    strcat(report, temp);

    msg = checkCoolantLevel(car.coolantLevel, &score, &status);
    escape_pango_markup(msg, escaped, sizeof(escaped));
    snprintf(temp, sizeof(temp), "%s%s%s\n", status == 0 ? GREEN : RED, escaped, RESET);
    strcat(report, temp);

    msg = checkBrakeFluidLevel(car.brakeFluidLevel, &score, &status);
    escape_pango_markup(msg, escaped, sizeof(escaped));
    snprintf(temp, sizeof(temp), "%s%s%s\n", status == 0 ? GREEN : RED, escaped, RESET);
    strcat(report, temp);

    msg = checkTransmissionFluidLevel(car.transmissionFluidLevel, &score, &status);
    escape_pango_markup(msg, escaped, sizeof(escaped));
    snprintf(temp, sizeof(temp), "%s%s%s\n", status == 0 ? GREEN : RED, escaped, RESET);
    strcat(report, temp);

    // Summary
    snprintf(temp, sizeof(temp), "Score: %d/100\n", score);
    escape_pango_markup(temp, escaped, sizeof(escaped));
    strcat(report, escaped);

    displayScoreBar(score, temp);
    escape_pango_markup(temp, escaped, sizeof(escaped));
    strcat(report, escaped);

    showHealthTip(score, temp, &status);
    escape_pango_markup(temp, escaped, sizeof(escaped));
    snprintf(temp, sizeof(temp), "%s%s%s\n", status == 0 ? GREEN : YELLOW, escaped, RESET);
    strcat(report, temp);

    // Debug: Print the report to console
    printf("Generated Report:\n%s\n", report);

    // Display report
    append_to_text_view(widgets->text_view, report);

    // Check for critical conditions
    if (car.engineTemp > 130) {
        sendSOS("Engine Overheating Critical");
        char message[1000];
        escape_pango_markup("Engine Overheating! SOS Sent!\n", message, sizeof(message));
        char formatted[1100];
        snprintf(formatted, sizeof(formatted), "%s%s%s", RED, message, RESET);
        append_to_text_view(widgets->text_view, formatted);
    }
    if (score < 40) {
        sendSOS("Vehicle Condition Critical");
        char message[1000];
        escape_pango_markup("Vehicle Critical! SOS Sent!\n", message, sizeof(message));
        char formatted[1100];
        snprintf(formatted, sizeof(formatted), "%s%s%s", RED, message, RESET);
        append_to_text_view(widgets->text_view, formatted);
    }
    if (car.frontBrakeWear <= 10 || car.rearBrakeWear <= 10) {
        sendSOS("Brake System Failure Imminent");
        char message[1000];
        escape_pango_markup("Brake Failure Imminent! SOS Sent!\n", message, sizeof(message));
        char formatted[1100];
        snprintf(formatted, sizeof(formatted), "%s%s%s", RED, message, RESET);
        append_to_text_view(widgets->text_view, formatted);
    }

    // Create dialog for saving report
    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Save Report?");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(widgets->window));
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 100);

    // Create a vertical box for content and buttons
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_start(box, 10);
    gtk_widget_set_margin_end(box, 10);
    gtk_widget_set_margin_top(box, 10);
    gtk_widget_set_margin_bottom(box, 10);
    gtk_window_set_child(GTK_WINDOW(dialog), box);

    // Add label
    GtkWidget *label = gtk_label_new("Save this report to a file?");
    gtk_box_append(GTK_BOX(box), label);

    // Add buttons
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(box), button_box);

    GtkWidget *no_button = gtk_button_new_with_label("No");
    g_object_set_data(G_OBJECT(no_button), "response", GINT_TO_POINTER(GTK_RESPONSE_NO));
    g_signal_connect(no_button, "clicked", G_CALLBACK(on_save_dialog_response), widgets);
    gtk_box_append(GTK_BOX(button_box), no_button);

    GtkWidget *yes_button = gtk_button_new_with_label("Yes");
    g_object_set_data(G_OBJECT(yes_button), "response", GINT_TO_POINTER(GTK_RESPONSE_YES));
    g_signal_connect(yes_button, "clicked", G_CALLBACK(on_save_dialog_response), widgets);
    gtk_box_append(GTK_BOX(button_box), yes_button);

    // Show the dialog
    gtk_window_present(GTK_WINDOW(dialog));
}

// Create main window
void create_main_window(GtkApplication *app, AppWidgets *widgets) {
    widgets->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(widgets->window), "Car Condition Analyzer v1.0");
    gtk_window_set_default_size(GTK_WINDOW(widgets->window), 1000, 800);

    GtkWidget *main_grid = gtk_grid_new();
    gtk_window_set_child(GTK_WINDOW(widgets->window), main_grid);
    gtk_grid_set_row_spacing(GTK_GRID(main_grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(main_grid), 10);
    gtk_widget_set_margin_start(main_grid, 10);
    gtk_widget_set_margin_end(main_grid, 10);
    gtk_widget_set_margin_top(main_grid, 10);
    gtk_widget_set_margin_bottom(main_grid, 10);

    gtk_grid_set_column_homogeneous(GTK_GRID(main_grid), TRUE);

    // Title
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b><big>CAR CONDITION ANALYZER</big></b>");
    gtk_grid_attach(GTK_GRID(main_grid), title, 0, 0, 2, 1);

    // Left column for inputs
    GtkWidget *input_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(input_vbox, GTK_ALIGN_START);
    gtk_widget_set_valign(input_vbox, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(main_grid), input_vbox, 0, 1, 1, 1);

    // Engine & Battery Section
    GtkWidget *label_engine = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_engine), "<b>Engine & Battery</b>");
    gtk_box_append(GTK_BOX(input_vbox), label_engine);

    GtkWidget *hbox_engine_temp = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(hbox_engine_temp), gtk_label_new("Engine Temperature (°C):"));
    widgets->entry_engine_temp = gtk_entry_new();
    g_signal_connect(widgets->entry_engine_temp, "changed", G_CALLBACK(on_input_changed), widgets);
    gtk_box_append(GTK_BOX(hbox_engine_temp), widgets->entry_engine_temp);
    gtk_box_append(GTK_BOX(input_vbox), hbox_engine_temp);

    widgets->check_engine_noise = gtk_check_button_new_with_label("Engine Noise Detected");
    g_signal_connect(widgets->check_engine_noise, "toggled", G_CALLBACK(on_input_changed), widgets);
    gtk_box_append(GTK_BOX(input_vbox), widgets->check_engine_noise);

    GtkWidget *hbox_battery_voltage = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(hbox_battery_voltage), gtk_label_new("Battery Voltage (V):"));
    widgets->entry_battery_voltage = gtk_entry_new();
    g_signal_connect(widgets->entry_battery_voltage, "changed", G_CALLBACK(on_input_changed), widgets);
    gtk_box_append(GTK_BOX(hbox_battery_voltage), widgets->entry_battery_voltage);
    gtk_box_append(GTK_BOX(input_vbox), hbox_battery_voltage);

    GtkWidget *hbox_battery_age = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(hbox_battery_age), gtk_label_new("Battery Age (years):"));
    widgets->entry_battery_age = gtk_entry_new();
    g_signal_connect(widgets->entry_battery_age, "changed", G_CALLBACK(on_input_changed), widgets);
    gtk_box_append(GTK_BOX(hbox_battery_age), widgets->entry_battery_age);
    gtk_box_append(GTK_BOX(input_vbox), hbox_battery_age);

    // Brakes & Tires Section
    GtkWidget *label_brakes = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_brakes), "<b>Brakes & Tires</b>");
    gtk_box_append(GTK_BOX(input_vbox), label_brakes);

    GtkWidget *hbox_front_brake = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(hbox_front_brake), gtk_label_new("Front Brake Wear (%):"));
    widgets->entry_front_brake = gtk_entry_new();
    g_signal_connect(widgets->entry_front_brake, "changed", G_CALLBACK(on_input_changed), widgets);
    gtk_box_append(GTK_BOX(hbox_front_brake), widgets->entry_front_brake);
    gtk_box_append(GTK_BOX(input_vbox), hbox_front_brake);

    GtkWidget *hbox_rear_brake = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(hbox_rear_brake), gtk_label_new("Rear Brake Wear (%):"));
    widgets->entry_rear_brake = gtk_entry_new();
    g_signal_connect(widgets->entry_rear_brake, "changed", G_CALLBACK(on_input_changed), widgets);
    gtk_box_append(GTK_BOX(hbox_rear_brake), widgets->entry_rear_brake);
    gtk_box_append(GTK_BOX(input_vbox), hbox_rear_brake);

    for (int i = 0; i < NUM_TIRES; i++) {
        char label_text[30];
        snprintf(label_text, sizeof(label_text), "Tire %d Pressure (PSI):", i + 1);
        GtkWidget *hbox_tire_pressure = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_box_append(GTK_BOX(hbox_tire_pressure), gtk_label_new(label_text));
        widgets->entry_tire_pressures[i] = gtk_entry_new();
        g_signal_connect(widgets->entry_tire_pressures[i], "changed", G_CALLBACK(on_input_changed), widgets);
        gtk_box_append(GTK_BOX(hbox_tire_pressure), widgets->entry_tire_pressures[i]);
        gtk_box_append(GTK_BOX(input_vbox), hbox_tire_pressure);
    }

    // Fluids Section
    GtkWidget *label_fluids = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_fluids), "<b>Fluids</b>");
    gtk_box_append(GTK_BOX(input_vbox), label_fluids);

    GtkWidget *hbox_oil_months = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(hbox_oil_months), gtk_label_new("Months Since Oil Change:"));
    widgets->entry_oil_months = gtk_entry_new();
    g_signal_connect(widgets->entry_oil_months, "changed", G_CALLBACK(on_input_changed), widgets);
    gtk_box_append(GTK_BOX(hbox_oil_months), widgets->entry_oil_months);
    gtk_box_append(GTK_BOX(input_vbox), hbox_oil_months);

    GtkWidget *hbox_oil_level = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(hbox_oil_level), gtk_label_new("Oil Level (%):"));
    widgets->entry_oil_level = gtk_entry_new();
    g_signal_connect(widgets->entry_oil_level, "changed", G_CALLBACK(on_input_changed), widgets);
    gtk_box_append(GTK_BOX(hbox_oil_level), widgets->entry_oil_level);
    gtk_box_append(GTK_BOX(input_vbox), hbox_oil_level);

    GtkWidget *hbox_coolant_level = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(hbox_coolant_level), gtk_label_new("Coolant Level (%):"));
    widgets->entry_coolant_level = gtk_entry_new();
    g_signal_connect(widgets->entry_coolant_level, "changed", G_CALLBACK(on_input_changed), widgets);
    gtk_box_append(GTK_BOX(hbox_coolant_level), widgets->entry_coolant_level);
    gtk_box_append(GTK_BOX(input_vbox), hbox_coolant_level);

    GtkWidget *hbox_brake_fluid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(hbox_brake_fluid), gtk_label_new("Brake Fluid Level (%):"));
    widgets->entry_brake_fluid = gtk_entry_new();
    g_signal_connect(widgets->entry_brake_fluid, "changed", G_CALLBACK(on_input_changed), widgets);
    gtk_box_append(GTK_BOX(hbox_brake_fluid), widgets->entry_brake_fluid);
    gtk_box_append(GTK_BOX(input_vbox), hbox_brake_fluid);

    GtkWidget *hbox_trans_fluid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(hbox_trans_fluid), gtk_label_new("Transmission Fluid Level (%):"));
    widgets->entry_trans_fluid = gtk_entry_new();
    g_signal_connect(widgets->entry_trans_fluid, "changed", G_CALLBACK(on_input_changed), widgets);
    gtk_box_append(GTK_BOX(hbox_trans_fluid), widgets->entry_trans_fluid);
    gtk_box_append(GTK_BOX(input_vbox), hbox_trans_fluid);

    // Buttons
    widgets->simulate_check = gtk_check_button_new_with_label("Simulate Inputs");
    g_signal_connect(widgets->simulate_check, "toggled", G_CALLBACK(on_simulate_toggled), widgets);
    gtk_box_append(GTK_BOX(input_vbox), widgets->simulate_check);

    widgets->diagnose_button = gtk_button_new_with_label("Diagnose");
    g_signal_connect(widgets->diagnose_button, "clicked", G_CALLBACK(on_diagnose_clicked), widgets);
    gtk_box_append(GTK_BOX(input_vbox), widgets->diagnose_button);

    // Right column for output
    GtkWidget *output_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(output_vbox, GTK_ALIGN_FILL);
    gtk_widget_set_valign(output_vbox, GTK_ALIGN_FILL);
    gtk_grid_attach(GTK_GRID(main_grid), output_vbox, 1, 1, 1, 1);

    GtkWidget *label_output = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_output), "<b>Diagnostic Report</b>");
    gtk_box_append(GTK_BOX(output_vbox), label_output);

    widgets->text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(widgets->text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(widgets->text_view), GTK_WRAP_WORD);
    gtk_widget_set_size_request(widgets->text_view, 500, 600);

    GtkWidget *scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), widgets->text_view);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_box_append(GTK_BOX(output_vbox), scrolled_window);

    gtk_window_present(GTK_WINDOW(widgets->window));
}

// Diagnostic functions
const char* checkEngineTemp(float temp, int* score, int* status) {
    static char buffer[100];
    if (temp < TEMP_LOW) {
        *score -= 10;
        *status = 1;  // Red
        snprintf(buffer, sizeof(buffer), "Engine Temp: %.1f°C (Low)", temp);
    } else if (temp > TEMP_HIGH) {
        *score -= 20;
        *status = 1;  // Red
        snprintf(buffer, sizeof(buffer), "Engine Temp: %.1f°C (High)", temp);
    } else {
        *status = 0;  // Green
        snprintf(buffer, sizeof(buffer), "Engine Temp: %.1f°C (OK)", temp);
    }
    return buffer;
}

const char* checkEngineNoise(int noiseFlag, int* score, int* status) {
    static char buffer[100];
    if (noiseFlag) {
        *score -= 15;
        *status = 1;  // Red
        snprintf(buffer, sizeof(buffer), "Engine Noise: Yes (Service)");
    } else {
        *status = 0;  // Green
        snprintf(buffer, sizeof(buffer), "Engine Noise: No (OK)");
    }
    return buffer;
}

const char* checkBatteryVoltage(float voltage, int* score, int* status) {
    static char buffer[100];
    if (voltage < BATTERY_VOLTAGE_MIN) {
        *score -= 10;
        *status = 1;  // Red
        snprintf(buffer, sizeof(buffer), "Battery Voltage: %.1fV (Low)", voltage);
    } else {
        *status = 0;  // Green
        snprintf(buffer, sizeof(buffer), "Battery Voltage: %.1fV (OK)", voltage);
    }
    return buffer;
}

const char* checkBatteryAge(int ageYears, int* score, int* status) {
    static char buffer[100];
    if (ageYears > BATTERY_AGE_MAX) {
        *score -= 10;
        *status = 1;  // Red
        snprintf(buffer, sizeof(buffer), "Battery Age: %d years (Replace)", ageYears);
    } else {
        *status = 0;  // Green
        snprintf(buffer, sizeof(buffer), "Battery Age: %d years (OK)", ageYears);
    }
    return buffer;
}

const char* evaluateBrakePad(const char* label, int wearPercent, int* score, int* status) {
    static char buffer[100];
    if (wearPercent <= 25) {
        *score -= 20;
        *status = 1;  // Red
        snprintf(buffer, sizeof(buffer), "%s Brake: %d%% (Replace)", label, wearPercent);
    } else if (wearPercent <= 50) {
        *score -= 10;
        *status = 2;  // Yellow
        snprintf(buffer, sizeof(buffer), "%s Brake: %d%% (Monitor)", label, wearPercent);
    } else {
        *status = 0;  // Green
        snprintf(buffer, sizeof(buffer), "%s Brake: %d%% (OK)", label, wearPercent);
    }
    return buffer;
}

int validatePressure(float p) {
    return p >= 0 && p <= 50;
}

TireStatus checkTirePressure(float pressures[NUM_TIRES]) {
    TireStatus status = { .imbalance = 0 };
    memcpy(status.pressures, pressures, sizeof(float) * NUM_TIRES);
    for (int i = 0; i < NUM_TIRES; i++) {
        status.warnings[i] = (pressures[i] < TIRE_PRESSURE_MIN || pressures[i] > TIRE_PRESSURE_MAX) ? 1 : 0;
    }
    for (int i = 0; i < NUM_TIRES - 1; i++) {
        for (int j = i + 1; j < NUM_TIRES; j++) {
            if (fabs(pressures[i] - pressures[j]) > PRESSURE_DIFF_THRESHOLD) {
                status.imbalance = 1;
                break;
            }
        }
        if (status.imbalance) break;
    }
    return status;
}

void generateTirePressureReport(TireStatus tireStatus, int* score, char* report) {
    char temp[500];
    char escaped[1000];
    for (int i = 0; i < NUM_TIRES; i++) {
        if (tireStatus.warnings[i]) {
            *score -= 10;
            snprintf(temp, sizeof(temp), "Tire %d: %.1f PSI (Out of Range)", i + 1, tireStatus.pressures[i]);
            escape_pango_markup(temp, escaped, sizeof(escaped));
            snprintf(temp, sizeof(temp), "%s%s%s\n", RED, escaped, RESET);
        } else {
            snprintf(temp, sizeof(temp), "Tire %d: %.1f PSI (OK)", i + 1, tireStatus.pressures[i]);
            escape_pango_markup(temp, escaped, sizeof(escaped));
            snprintf(temp, sizeof(temp), "%s%s%s\n", GREEN, escaped, RESET);
        }
        strcat(report, temp);
    }
    if (tireStatus.imbalance) {
        *score -= 10;
        snprintf(temp, sizeof(temp), "Tire Imbalance: Greater than %.1f PSI difference", PRESSURE_DIFF_THRESHOLD);
        escape_pango_markup(temp, escaped, sizeof(escaped));
        snprintf(temp, sizeof(temp), "%s%s%s\n", RED, escaped, RESET);
        strcat(report, temp);
    }
}

const char* checkOilChangeStatus(int months, int* score, int* status) {
    static char buffer[100];
    if (months > OIL_MONTHS_MAX) {
        *score -= 10;
        *status = 1;  // Red
        snprintf(buffer, sizeof(buffer), "Oil Change: %d months (Due)", months);
    } else {
        *status = 0;  // Green
        snprintf(buffer, sizeof(buffer), "Oil Change: %d months (OK)", months);
    }
    return buffer;
}

const char* checkOilLevel(int level, int* score, int* status) {
    static char buffer[100];
    if (level < OIL_LEVEL_MIN) {
        *score -= 10;
        *status = 1;  // Red
        snprintf(buffer, sizeof(buffer), "Oil Level: %d%% (Low)", level);
    } else {
        *status = 0;  // Green
        snprintf(buffer, sizeof(buffer), "Oil Level: %d%% (OK)", level);
    }
    return buffer;
}

const char* checkCoolantLevel(int level, int* score, int* status) {
    static char buffer[100];
    if (level < COOLANT_LEVEL_MIN) {
        *score -= 10;
        *status = 1;  // Red
        snprintf(buffer, sizeof(buffer), "Coolant: %d%% (Low)", level);
    } else {
        *status = 0;  // Green
        snprintf(buffer, sizeof(buffer), "Coolant: %d%% (OK)", level);
    }
    return buffer;
}

const char* checkBrakeFluidLevel(int level, int* score, int* status) {
    static char buffer[100];
    if (level < BRAKE_FLUID_MIN) {
        *score -= 10;
        *status = 1;  // Red
        snprintf(buffer, sizeof(buffer), "Brake Fluid: %d%% (Low)", level);
    } else {
        *status = 0;  // Green
        snprintf(buffer, sizeof(buffer), "Brake Fluid: %d%% (OK)", level);
    }
    return buffer;
}

const char* checkTransmissionFluidLevel(int level, int* score, int* status) {
    static char buffer[100];
    if (level < TRANSMISSION_FLUID_MIN) {
        *score -= 10;
        *status = 1;  // Red
        snprintf(buffer, sizeof(buffer), "Trans Fluid: %d%% (Low)", level);
    } else {
        *status = 0;  // Green
        snprintf(buffer, sizeof(buffer), "Trans Fluid: %d%% (OK)", level);
    }
    return buffer;
}

void displayScoreBar(int score, char* buffer) {
    int bars = score / 10;
    char bar[50] = "";
    for (int i = 0; i < 10; i++) {
        strcat(bar, i < bars ? "#" : "-");
    }
    snprintf(buffer, 50, "[%s] %d%%\n", bar, score);
}

void showHealthTip(int score, char* buffer, int* status) {
    if (score < 50) {
        *status = 1;  // Yellow
        snprintf(buffer, 100, "Urgent: Needs immediate maintenance.");
    } else if (score < 80) {
        *status = 1;  // Yellow
        snprintf(buffer, 100, "Monitor: Schedule maintenance soon.");
    } else {
        *status = 0;  // Green
        snprintf(buffer, 100, "Good: Continue regular maintenance.");
    }
}

void sendSOS(const char* reason) {
    printf("SOS Alert: %s\n", reason);
}

void saveCSV(const char* report) {
    FILE *file = fopen("vehicle_report.csv", "w");
    if (file) {
        fprintf(file, "Component,Status\n");
        char *copy = strdup(report);
        char *line = strtok(copy, "\n");
        while (line) {
            if (strstr(line, "Engine Temp") || strstr(line, "Battery Voltage") ||
                strstr(line, "Tire") || strstr(line, "Oil") || strstr(line, "Brake") ||
                strstr(line, "Coolant") || strstr(line, "Trans Fluid")) {
                char plain[1000] = "";
                int j = 0;
                for (int i = 0; line[i] != '\0' && j < sizeof(plain) - 1; i++) {
                    if (line[i] == '<') {
                        while (line[i] != '>' && line[i] != '\0') i++;
                    } else {
                        plain[j++] = line[i];
                    }
                }
                plain[j] = '\0';
                fprintf(file, "%s\n", plain);
            }
            line = strtok(NULL, "\n");
        }
        free(copy);
        fclose(file);
    }
}

// Main activation function
static void activate(GtkApplication *app, gpointer user_data) {
    AppWidgets *widgets = (AppWidgets *)user_data;
    create_main_window(app, widgets);
}

// Main function
int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.car.analyzer", G_APPLICATION_DEFAULT_FLAGS);
    AppWidgets widgets = {0};

    g_signal_connect(app, "activate", G_CALLBACK(activate), &widgets);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}

