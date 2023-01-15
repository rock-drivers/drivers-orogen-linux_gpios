# frozen_string_literal: true

require "pathname"

using_task_library "linux_gpios"

describe OroGen.linux_gpios.Task do
    run_live

    attr_reader :task

    before do
        @gpio_root = Pathname(Dir.mktmpdir)

        @task = syskit_deploy(
            OroGen.linux_gpios.Task
                  .deployed_as("task_under_test")
        )
        @task.properties.sysfs_gpio_path = @gpio_root.to_s
    end

    after do
        @gpio_root&.rmtree
    end

    it "fails to configure if a GPIO does not exist" do
        task.properties.r_configuration = { ids: [124] }
        assert_raises(Roby::EmissionFailed) do
            syskit_configure(task)
        end
    end

    it "fails to configure if a GPIO does not exist" do
        task.properties.r_configuration = { ids: [124] }
        assert_raises(Roby::EmissionFailed) do
            syskit_configure(task)
        end
    end

    it "outputs the initial state of the GPIO on start" do
        make_fake_gpio(124, false)
        task.properties.r_configuration = { ids: [124] }
        syskit_configure(task)

        sample = expect_execution { task.start! }.to do
            have_one_new_sample task.r_states_port
        end

        assert_equal [0], sample.states.map(&:data)
    end

    it "writes a new state and reads it back" do
        make_fake_gpio(124, false)
        task.properties.w_configuration = { ids: [124] }
        task.properties.r_configuration = { ids: [124] }
        syskit_configure_and_start(task)

        command = { states: [{ data: 1 }] }
        expect_execution { syskit_write(task.w_commands_port, command) }.to do
            have_one_new_sample(task.r_states_port)
                .matching { |s| s.states[0].data != 0 }
        end

        assert read_fake_gpio(124)
    end

    it "outputs the current state periodically by default" do
        make_fake_gpio(124, false)
        task.properties.r_configuration = { ids: [124] }
        syskit_configure_and_start(task)

        expect_execution.to { have_new_samples(task.r_states_port, 10) }
    end

    it "does not write if there are no state changes when edge_triggered_output is set" do
        make_fake_gpio(124, false)
        task.properties.edge_triggered_output = true
        task.properties.r_configuration = { ids: [124] }
        syskit_configure_and_start(task)

        expect_execution.to do
            have_no_new_sample(task.r_states_port, at_least_during: 1)
        end
    end

    it "rejects a command input whose size is lower than the expected" do
        make_fake_gpio(124, false)
        task.properties.w_configuration = { ids: [124] }
        syskit_configure_and_start(task)

        command = { states: [] }
        expect_execution { syskit_write(task.w_commands_port, command) }.to do
            emit task.unexpected_command_size_event
        end
    end

    it "rejects a command input whose size is greater than the expected" do
        make_fake_gpio(124, false)
        task.properties.w_configuration = { ids: [124] }
        syskit_configure_and_start(task)

        command = { states: [{ data: 1 }, { data: 1 }] }
        expect_execution { syskit_write(task.w_commands_port, command) }.to do
            emit task.unexpected_command_size_event
        end
    end

    it "with edge_triggered_output set, writes the current value when it receives "\
       "a command, regardless of whether the value changed" do
        make_fake_gpio(124, false)
        task.properties.w_configuration = { ids: [124] }
        task.properties.r_configuration = { ids: [124] }
        task.properties.edge_triggered_output = true
        syskit_configure_and_start(task)

        r_states_reader = syskit_create_reader task.r_states_port
        command = { states: [{ data: 0 }] }
        sample = expect_execution { syskit_write(task.w_commands_port, command) }.to do
            have_one_new_sample(r_states_reader)
        end

        assert_equal 0, sample.states[0].data
    end

    it "with edge_triggered_output set, writes the default value on start and "\
       "reports the initial value if it changed" do
        make_fake_gpio(124, false)
        task.properties.w_configuration = {
            ids: [124], defaults: [1], timeout: Time.at(20)
        }
        task.properties.r_configuration = { ids: [124] }
        task.properties.edge_triggered_output = true
        syskit_configure(task)

        samples = expect_execution { task.start! }.to do
            have_new_samples(task.r_states_port, 2)
        end

        assert read_fake_gpio(124)
        assert_equal 0, samples[0].states[0].data
        assert_equal 1, samples[1].states[0].data
    end

    it "writes the default value if the port is disconnected" do
        make_fake_gpio(124, false)
        task.properties.w_configuration = {
            ids: [124], defaults: [0], timeout: Time.at(600)
        }
        syskit_configure_and_start(task)

        w = syskit_create_writer(task.w_commands_port)

        command = { states: [{ data: 1 }] }
        expect_execution { syskit_write(w, command) }.to do
            achieve { read_fake_gpio(124) }
        end
        w.disconnect

        expect_execution.to do
            achieve { !read_fake_gpio(124) }
        end
    end

    it "writes the default value if no new samples "\
       "are received within the configured timeout" do
        make_fake_gpio(124, false)
        task.properties.w_configuration = {
            ids: [124], defaults: [0], timeout: Time.at(0.5)
        }
        syskit_configure_and_start(task)

        w = syskit_create_writer(task.w_commands_port)

        command = { states: [{ data: 1 }] }
        expect_execution { syskit_write(w, command) }.to do
            achieve { read_fake_gpio(124) }
        end

        sleep(0.8)
        refute read_fake_gpio(124)
    end

    def make_fake_gpio(id, value)
        (@gpio_root / "gpio#{id}").mkpath
        write_fake_gpio(id, value)
    end

    def write_fake_gpio(id, value)
        (@gpio_root / "gpio#{id}" / "value").write(value ? "1" : "0")
    end

    def read_fake_gpio(id)
        (@gpio_root / "gpio#{id}" / "value").read == "1"
    end
end