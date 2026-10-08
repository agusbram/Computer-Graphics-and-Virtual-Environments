#include <iostream>
#include <vector>
#include <fstream>

#include <glm/glm.hpp>

#include <dlfdm/defines.h>
#include <dlfdm/fdmsolver.h>
#include <dlfdm/aerodynamicsmodel.h>
#include <dlfdm/models/aircraft/jettrainer.h>

void LoadPhugoidInput(std::vector<glm::vec2>& control_inputs,
                      const float& begin_time,
                      const float& neutral_input)
{
        glm::vec2 elevator_input;
        constexpr float kInputDuration = 0.15f;     // [seg]

        // Doblete de elevador para excitar modo Fugoide
        elevator_input = glm::vec2(begin_time,glm::radians(-10.0f));
        control_inputs.push_back(elevator_input);
        elevator_input = glm::vec2(begin_time + kInputDuration ,glm::radians(10.0f));
        control_inputs.push_back(elevator_input);
        elevator_input = glm::vec2(begin_time + 2 * kInputDuration, neutral_input);
        control_inputs.push_back(elevator_input);
}

void LoadAileronInput(std::vector<glm::vec2>& control_inputs,
                      const float& begin_time,
                      const float& neutral_input)
{
        glm::vec2 aileron_input;
        constexpr float kInputDuration = 0.15f;     // [seg]

        // Deflexión de 5 deg y regreso a posición neutra
        aileron_input = glm::vec2(begin_time,glm::radians(5.0f));
        control_inputs.push_back(aileron_input);
        aileron_input = glm::vec2(begin_time + kInputDuration, neutral_input);
        control_inputs.push_back(aileron_input);
}

void LoadRudderInput(std::vector<glm::vec2>& control_inputs,
                     const float& begin_time,
                     const float& neutral_input)
{
        glm::vec2 rudder_input;
        constexpr float kInputDuration = 0.25f;     // [seg]

        // Doblete de timón de dirección
        rudder_input = glm::vec2(begin_time,glm::radians(-5.0f));
        control_inputs.push_back(rudder_input);
        rudder_input = glm::vec2(begin_time + kInputDuration ,glm::radians(5.0f));
        control_inputs.push_back(rudder_input);
        rudder_input = glm::vec2(begin_time + 2 * kInputDuration, neutral_input);
        control_inputs.push_back(rudder_input);
}

int main()
{
    dlfdm::AircraftParameters aermacchi_s211 = dlfdm::jettrainer::load_model();

    dlfdm::FDMSolver fdm(aermacchi_s211);

    // Initial conditions for integration: state and controls come together,
    // see dlfdm::TrimPoint
    dlfdm::TrimPoint trim = dlfdm::jettrainer::get_trim_condition(
                dlfdm::jettrainer::TrimCondition::kISA5000TAS150);

    dlfdm::AircraftState init_state = trim.state;
    dlfdm::ControlInputs controls = trim.controls;

    std::cout << "DLFDM" << std::endl;
    std::cout << "----------------------" << std::endl;

    // Load a control input for testing the fdm
    typedef enum ControlsId {kNone = 0, kThrottle, kElevator, kAileron, kRudder } ControlsId;

    std::vector<glm::vec2> control_inputs;
    control_inputs.clear();

    ControlsId use_control_input = ControlsId::kNone;

    switch (use_control_input) {
    case kNone:
        break;
    case kThrottle:
        break;
    case kElevator:
        LoadPhugoidInput(control_inputs,2.5f,controls.elevator);
        break;
    case kAileron:
        LoadAileronInput(control_inputs,2.5f,0.0f);
        break;
    case kRudder:
        LoadRudderInput(control_inputs,2.5f,0.0f);
        break;
    }

    // Total sim time
    const float sim_time = 1.0f * 60.0f;
    const unsigned long steps = static_cast<unsigned long>(sim_time * 120.0f);

    // Load initial conditions (trim) for the solver
    fdm.setState(init_state);

    // Prepare controls input for the loop execution
    size_t current_input = 0;
    glm::vec2 control_input = glm::vec2(0.0f,0.0f);

    // Process inputs if any
    if( control_inputs.size() > 0){
        control_input = control_inputs.at(current_input++);
    }

    // Logging file
    std::fstream logging_file;
    logging_file.open("salida.csv",std::ios_base::out);

    if( !logging_file.is_open() ){
        std::cerr << "Failed to open logging file!" << std::endl;
        return -1;
    }

    // Do some logging
    fdm.log_titles(logging_file);
    fdm.log_state(logging_file);

    std::cout << " Sim time: " << sim_time << " seg" << std::endl;
    std::cout << " Beginning fdm execution ..." << std::endl;

    // Main FDM update
    for (unsigned long i = 0 ; i < steps ; i++) {

        // Update controls inputs
        if( control_inputs.size() > 0 ){
            if( fdm.get_sim_time() > control_input.x ){
                switch (use_control_input) {
                case kNone:
                    break;
                case kThrottle:
                    controls.throttle = control_input.y;
                    break;
                case kElevator:
                    controls.elevator = control_input.y;
                    break;
                case kAileron:
                    controls.aileron = control_input.y;
                    break;
                case kRudder:
                    controls.rudder = control_input.y;
                    break;
                }

                // read next input to be processed
                if( current_input < control_inputs.size() ){
                    control_input = control_inputs.at(current_input++);
                }
            }
        }

        fdm.update(controls);

        // Do data logging
        fdm.log_state(logging_file);
    }

    std::cout << " Done!" << std::endl;

    logging_file.close();

    return 0;
}
