require "application_system_test_case"

class ComputadorsTest < ApplicationSystemTestCase
  setup do
    @computador = computadors(:one)
  end

  test "visiting the index" do
    visit computadors_url
    assert_selector "h1", text: "Computadors"
  end

  test "should create computador" do
    visit computadors_url
    click_on "New computador"

    fill_in "Marca", with: @computador.marca
    fill_in "Numserie", with: @computador.numserie
    fill_in "Pessoa", with: @computador.pessoa_id
    click_on "Create Computador"

    assert_text "Computador was successfully created"
    click_on "Back"
  end

  test "should update Computador" do
    visit computador_url(@computador)
    click_on "Edit this computador", match: :first

    fill_in "Marca", with: @computador.marca
    fill_in "Numserie", with: @computador.numserie
    fill_in "Pessoa", with: @computador.pessoa_id
    click_on "Update Computador"

    assert_text "Computador was successfully updated"
    click_on "Back"
  end

  test "should destroy Computador" do
    visit computador_url(@computador)
    click_on "Destroy this computador", match: :first

    assert_text "Computador was successfully destroyed"
  end
end
