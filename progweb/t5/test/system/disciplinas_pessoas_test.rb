require "application_system_test_case"

class DisciplinasPessoasTest < ApplicationSystemTestCase
  setup do
    @disciplinas_pessoa = disciplinas_pessoas(:one)
  end

  test "visiting the index" do
    visit disciplinas_pessoas_url
    assert_selector "h1", text: "Disciplinas pessoas"
  end

  test "should create disciplinas pessoa" do
    visit disciplinas_pessoas_url
    click_on "New disciplinas pessoa"

    fill_in "Disciplina", with: @disciplinas_pessoa.disciplina_id
    fill_in "Pessoa", with: @disciplinas_pessoa.pessoa_id
    click_on "Create Disciplinas pessoa"

    assert_text "Disciplinas pessoa was successfully created"
    click_on "Back"
  end

  test "should update Disciplinas pessoa" do
    visit disciplinas_pessoa_url(@disciplinas_pessoa)
    click_on "Edit this disciplinas pessoa", match: :first

    fill_in "Disciplina", with: @disciplinas_pessoa.disciplina_id
    fill_in "Pessoa", with: @disciplinas_pessoa.pessoa_id
    click_on "Update Disciplinas pessoa"

    assert_text "Disciplinas pessoa was successfully updated"
    click_on "Back"
  end

  test "should destroy Disciplinas pessoa" do
    visit disciplinas_pessoa_url(@disciplinas_pessoa)
    click_on "Destroy this disciplinas pessoa", match: :first

    assert_text "Disciplinas pessoa was successfully destroyed"
  end
end
