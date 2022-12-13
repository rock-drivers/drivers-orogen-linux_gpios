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

        10.times do
            expect_execution.to { have_one_new_sample(task.r_states_port) }
        end
    end

    it "writes only on state change if edge_triggered_output is set" do
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