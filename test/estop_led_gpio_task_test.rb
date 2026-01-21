# frozen_string_literal: true

using_task_library "linux_gpios"

describe OroGen.linux_gpios.EStopLedGPIOTask do
    run_live

    attr_reader :task

    before do
        @task = syskit_deploy(
            OroGen.linux_gpios.EStopLedGPIOTask
                .deployed_as("estop_led_gpio_task_test")
        )
        @task.properties.unknown_state_blink_period = Time.at(0.2)
        @task.properties.transition_blink_period = Time.at(0.5)
        syskit_configure_and_start(task)
    end

    describe "actual status is UNKNOWN" do
        it "toggles the gpio state according the unknown state period when the desired "\
           "status is OPERATIONAL" do
            @actual = { time: Time.now, safety_mode: :UNKNOWN }
            @desired = { time: Time.now, safety_mode: :OPERATIONAL }
        end

        it "toggles the gpio state according the unknown state period when the desired "\
            "status is DISABLED" do
            @actual = { time: Time.now, safety_mode: :UNKNOWN }
            @desired = { time: Time.now, safety_mode: :DISABLED }
        end

        after do
            output0 = expect_execution.poll do
                syskit_write @task.actual_status_port, @actual
                syskit_write @task.desired_status_port, @desired
            end.to do
                have_one_new_sample(task.gpio_state_port)
                    .matching { |s| s.states[0].data == 1 }
            end
            output1 = expect_execution.poll do
                syskit_write @task.actual_status_port, @actual
                syskit_write @task.desired_status_port, @desired
            end.to do
                have_one_new_sample(task.gpio_state_port)
                    .matching { |s| s.states[0].data == 0 }
            end
            assert_in_delta(output1.time, output0.time + 0.2, 1e-2)
        end
    end

    describe "desired status matches the actual status" do
        it "sets the gpio state to true when the status is DISABLED" do
            actual = { time: Time.now, safety_mode: :DISABLED }
            desired = { time: Time.now, safety_mode: :DISABLED }
            outputs = expect_execution do
                syskit_write @task.actual_status_port, actual
                syskit_write @task.desired_status_port, desired
            end.to do
                have_one_new_sample(task.gpio_state_port)
            end
            assert_equal(1, outputs.states[0].data)
        end

        it "sets the gpio state to false when the status is OPERATIONAL" do
            actual = { time: Time.now, safety_mode: :OPERATIONAL }
            desired = { time: Time.now, safety_mode: :OPERATIONAL }
            outputs = expect_execution do
                syskit_write @task.actual_status_port, actual
                syskit_write @task.desired_status_port, desired
            end.to do
                have_one_new_sample(task.gpio_state_port)
            end
            assert_equal(0, outputs.states[0].data)
        end

        it "sets the gpio state to false when the status is EMERGENCY_BEHAVIOR" do
            actual = { time: Time.now, safety_mode: :EMERGENCY_BEHAVIOR }
            desired = { time: Time.now, safety_mode: :EMERGENCY_BEHAVIOR }
            outputs = expect_execution do
                syskit_write @task.actual_status_port, actual
                syskit_write @task.desired_status_port, desired
            end.to do
                have_one_new_sample(task.gpio_state_port)
            end
            assert_equal(0, outputs.states[0].data)
        end
    end

    describe "desired status does not match the actual status" do
        it "toggles the gpio state according the transition state period when the "\
           "actual status is OPERATIONAL and desired is DISABLED" do
            @actual = { time: Time.now, safety_mode: :OPERATIONAL }
            @desired = { time: Time.now, safety_mode: :DISABLED }
        end

        it "toggles the gpio state according the transition state period when the "\
           "actual status is DISABLED and desired is OPERATIONAL" do
            @actual = { time: Time.now, safety_mode: :DISABLED }
            @desired = { time: Time.now, safety_mode: :OPERATIONAL }
        end

        it "toggles the gpio state according the transition state period when the "\
            "actual status is EMERGENCY_BEHAVIOR and desired is OPERATIONAL" do
            @actual = { time: Time.now, safety_mode: :EMERGENCY_BEHAVIOR }
            @desired = { time: Time.now, safety_mode: :OPERATIONAL }
        end

        it "toggles the gpio state according the transition state period when the "\
            "actual status is OPERATIONAL and desired is EMERGENCY_BEHAVIOR" do
            @actual = { time: Time.now, safety_mode: :OPERATIONAL }
            @desired = { time: Time.now, safety_mode: :EMERGENCY_BEHAVIOR }
        end

        after do
            output0 = expect_execution.poll do
                syskit_write @task.actual_status_port, @actual
                syskit_write @task.desired_status_port, @desired
            end.to do
                have_one_new_sample(task.gpio_state_port)
                    .matching { |s| s.states[0].data == 1 }
            end
            output1 = expect_execution.poll do
                syskit_write @task.actual_status_port, @actual
                syskit_write @task.desired_status_port, @desired
            end.to do
                have_one_new_sample(task.gpio_state_port)
                    .matching { |s| s.states[0].data == 0 }
            end
            assert_in_delta(output1.time, output0.time + 0.5, 1e-2)
        end
    end
end
