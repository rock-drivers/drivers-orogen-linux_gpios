# frozen_string_literal: true

using_task_library "linux_gpios"

describe OroGen.linux_gpios.TimerGPIOTask do
    run_live

    attr_reader :task, :off_state, :on_state

    before do
        @on_state = create_message(data: true)
        @off_state = create_message(data: false)
    end

    describe "normal operations" do
        it "activates the output for the configured amount seconds and deactivates it" do
            @task = create_configure_and_start_task(duration: 2)
            tic = Time.now

            output = expect_execution.to do
                have_one_new_sample(task.gpio_state_port)
            end
            assert_equal 1, output.states[0].data

            feedback_writer = syskit_create_writer(task.feedback_port)

            expect_execution
                .poll { feedback_writer.write(on_state) }
                .to do
                    have_one_new_sample(task.gpio_state_port)
                        .matching { |v| v.states[0].data == 0 }
                    have_one_new_sample(task.deadline_port)
                end
            toc = Time.now

            expect_execution
                .poll { feedback_writer.write(off_state) }
                .to { emit task.success_event }

            assert (toc - tic) > 1.5,
                   "expected the true state to be sent for at least 1.5 seconds, "\
                   "but got #{toc - tic}"
        end
    end

    describe "treating errors" do
        it "throws an error when there is no feedback at start hook" do
            @task = create_configure_task(duration: 2, switch_timeout: 1)

            expect_execution { task.start! }
                .join_all_waiting_work(false)
                .to do
                    have_one_new_sample(task.gpio_state_port)
                        .matching { |v| v.states[0].data == 0 }
                    fail_to_start task
                end
        end

        it "throws an error if there is no feedback during execution" do
            @task = create_configure_and_start_task(duration: 2)

            feedback_writer = syskit_create_writer(task.feedback_port)
            expect_execution
                .to do
                    have_one_new_sample(task.gpio_state_port)
                        .matching { |v| v.states[0].data == 0 }
                end

            expect_execution
                .poll { feedback_writer.write(off_state) }
                .to { emit task.exception_event }
        end

        it "throws an error in case of divergence between setpoint and feedback" do
            @task = create_configure_and_start_task(duration: 2)

            feedback_writer = syskit_create_writer(task.feedback_port)
            expect_execution
                .poll { feedback_writer.write(off_state) }
                .to do
                    have_one_new_sample(task.gpio_state_port)
                        .matching { |v| v.states[0].data == 0 }
                    emit task.exception_event
                end
        end

        it "throws an error after finishing the command and the task is not able to "\
           "switch back to the default output" do
            @task = create_configure_and_start_task(duration: 2)
            feedback_writer = syskit_create_writer(task.feedback_port)

            expect_execution
                .poll { feedback_writer.write(on_state) }
                .to do
                    have_one_new_sample(task.gpio_state_port)
                        .matching { |v| v.states[0].data == 0 }
                end

            expect_execution
                .poll { feedback_writer.write(on_state) }
                .to { emit task.exception_event }
        end
    end

    def create_task(duration:, switch_timeout: 2)
        task = syskit_deploy(
            OroGen.linux_gpios.TimerGPIOTask
                  .deployed_as("timer_gpio_test")
        )

        task.properties.deadline_report = Time.at(1)
        task.properties.feedback_timeout = Time.at(0.5)
        task.properties.switch_timeout = Time.at(switch_timeout)
        task.properties.set_state = true
        task.properties.duration = Time.at(duration)
        task
    end

    def create_configure_task(duration:, switch_timeout: 2)
        task = create_task(duration: duration, switch_timeout: switch_timeout)
        syskit_configure(task)
        task
    end

    def create_configure_and_start_task(duration:, switch_timeout: 2)
        task = create_configure_task(duration: duration, switch_timeout: switch_timeout)
        start_task(task)
        task
    end

    def start_task(task)
        feedback_writer = syskit_create_writer(task.feedback_port)
        expect_execution { task.start! }
            .join_all_waiting_work(false)
            .poll { feedback_writer.write(on_state) }
            .to { emit task.start_event }
    end

    def create_message(data:)
        msg = Types.linux_gpios.GPIOState.new
        msg.time = Time.now
        msg.states = [{ time: Time.now, data: data }]
        msg
    end
end
