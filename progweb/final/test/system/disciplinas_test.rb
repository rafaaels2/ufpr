require "application_system_test_case"

class DisciplinasTest < ApplicationSystemTestCase
  setup do
    @disciplina = disciplinas(:one)
  end

  test "visiting the index" do
    visit disciplinas_url
    assert_selector "h1", text: "Disciplinas"
  end

  test "should create disciplina" do
    visit disciplinas_url
    click_on "New disciplina"

    fill_in "Codigo", with: @disciplina.codigo
    fill_in "Nome", with: @disciplina.nome
    click_on "Create Disciplina"

    assert_text "Disciplina was successfully created"
    click_on "Back"
  end

  test "should update Disciplina" do
    visit disciplina_url(@disciplina)
    click_on "Edit this disciplina", match: :first

    fill_in "Codigo", with: @disciplina.codigo
    fill_in "Nome", with: @disciplina.nome
    click_on "Update Disciplina"

    assert_text "Disciplina was successfully updated"
    click_on "Back"
  end

  test "should destroy Disciplina" do
    visit disciplina_url(@disciplina)
    click_on "Destroy this disciplina", match: :first

    assert_text "Disciplina was successfully destroyed"
  end
end
