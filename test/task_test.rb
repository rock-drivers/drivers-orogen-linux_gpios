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

    it "writes and reads new state on the GPIO" do
        make_fake_gpio(124, "0")
        task.properties.w_configuration = { ids: [124] }
        task.properties.r_configuration = { ids: [124] }
        syskit_configure_and_start(task)

        command = { states: [{ data: 1 }] }
        expect_execution { syskit_write(task.w_commands_port, command) }.to do
            have_one_new_sample(task.r_states_port)
                .matching { |s| s.states[0].data == 1 }
        end

        assert_equal "1", read_fake_gpio(124)
    end

    def make_fake_gpio(id, value)
        (@gpio_root / "gpio#{id}").mkpath
        write_fake_gpio(id, value)
    end

    def write_fake_gpio(id, value)
        (@gpio_root / "gpio#{id}" / "value").write(value ? "1" : "0")
    end

    def read_fake_gpio(id)
        (@gpio_root / "gpio#{id}" / "value").read
    end
end